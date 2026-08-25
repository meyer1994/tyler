// wifi.hpp

#pragma once

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include <esp_pthread.h>
#include "./result.hpp"

struct WifiResponse {
    int code;
    String body;
};

class Wifi {
public:
    using ErrorCallback = std::function<void(const Error&)>;

    Wifi() = default;

    Wifi(String ssid, String password)
        : ssid_(std::move(ssid)),
          password_(std::move(password)) {}

    ~Wifi() {
        auto stopped = stop();
        if (stopped.valid()) stopped.wait();
    }

    Wifi(const Wifi&) = delete;
    Wifi& operator=(const Wifi&) = delete;

    Wifi& configure(String ssid, String password) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (running_)
            throw std::logic_error("cannot configure WiFi while running");
        ssid_ = std::move(ssid);
        password_ = std::move(password);
        return *this;
    }

    Wifi& onError(ErrorCallback callback) {
        if (callback) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            error_callbacks_.push_back(std::move(callback));
        }
        return *this;
    }

    std::future<Result<bool>> start() {
        auto promise = std::make_shared<std::promise<Result<bool>>>();
        auto future = promise->get_future();

        std::unique_lock<std::mutex> lock(state_mutex_);
        if (running_) {
            promise->set_value(true);
            return future;
        }
        if (worker_.joinable()) {
            Error error("WiFi must be stopped before it can restart");
            lock.unlock();
            emitError(error);
            promise->set_value(tl::make_unexpected(error));
            return future;
        }
        if (ssid_.isEmpty()) {
            Error error("WiFi is not configured");
            lock.unlock();
            emitError(error);
            promise->set_value(tl::make_unexpected(error));
            return future;
        }

        stop_requested_ = false;
        running_ = true;
        try {
            worker_ = makeThread(
                [this, promise] { workerLoop(promise); },
                "wifi", 16384, 1);
        } catch (const std::exception& exception) {
            running_ = false;
            Error error(exception.what());
            lock.unlock();
            emitError(error);
            promise->set_value(tl::make_unexpected(error));
        }
        return future;
    }

    std::future<Result<bool>> stop() {
        std::thread worker;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!worker_.joinable()) return readyFuture<bool>(true);
            stop_requested_ = true;
            command_cv_.notify_all();
            worker = std::move(worker_);
        }

        auto worker_to_join =
            std::make_shared<std::thread>(std::move(worker));
        std::packaged_task<Result<bool>()> task(
            [worker_to_join]() -> Result<bool> {
                worker_to_join->join();
                return true;
            });
        auto future = task.get_future();

        try {
            auto joiner = makeThread(
                std::move(task), "wifi_stop", 3072, 1);
            joiner.detach();
        } catch (const std::exception& exception) {
            if (worker_to_join->joinable()) worker_to_join->join();
            Error error(exception.what());
            emitError(error);
            return readyFuture<bool>(tl::make_unexpected(error));
        }
        return future;
    }

    std::future<Result<WifiResponse>> request(
        String method, String path, String body = String()) {

        auto promise = std::make_shared<
            std::promise<Result<WifiResponse>>>();
        auto future = promise->get_future();

        {
            std::lock_guard<std::mutex> lock(command_mutex_);
            if (!running_ || stop_requested_) {
                promise->set_value(
                    tl::make_unexpected(Error("WiFi is not running")));
                return future;
            }
            if (commands_.size() >= MAX_PENDING_REQUESTS) {
                promise->set_value(
                    tl::make_unexpected(Error("WiFi request queue is full")));
                return future;
            }
            commands_.push_back(Command{
                std::move(method), std::move(path), std::move(body), promise});
        }
        command_cv_.notify_one();
        return future;
    }

    Result<String> ip() const {
        if (WiFi.status() != WL_CONNECTED)
            return tl::make_unexpected(Error("not connected"));
        return WiFi.localIP().toString();
    }

private:
    static constexpr size_t MAX_PENDING_REQUESTS = 16;

    struct Command {
        String method;
        String path;
        String body;
        std::shared_ptr<std::promise<Result<WifiResponse>>> promise;
    };

    template <typename T>
    static std::future<Result<T>> readyFuture(Result<T> result) {
        std::promise<Result<T>> promise;
        auto future = promise.get_future();
        promise.set_value(std::move(result));
        return future;
    }

    template <typename Fn>
    static std::thread makeThread(
        Fn&& fn,
        const char* name,
        size_t stack_size,
        size_t priority) {

        esp_pthread_cfg_t previous{};
        const bool had_previous = esp_pthread_get_cfg(&previous) == ESP_OK;
        const esp_pthread_cfg_t restore =
            had_previous ? previous : esp_pthread_get_default_config();

        esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
        cfg.stack_size = stack_size;
        cfg.prio = priority;
        cfg.inherit_cfg = false;
        cfg.thread_name = name;
        cfg.pin_to_core = tskNO_AFFINITY;

        if (esp_pthread_set_cfg(&cfg) != ESP_OK)
            throw std::runtime_error("WiFi thread configuration failed");

        try {
            std::thread thread(std::forward<Fn>(fn));
            esp_pthread_set_cfg(&restore);
            return thread;
        } catch (...) {
            esp_pthread_set_cfg(&restore);
            throw;
        }
    }

    void workerLoop(
        const std::shared_ptr<std::promise<Result<bool>>>& started) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_.c_str(), password_.c_str());

        const uint32_t begin_ms = millis();
        while (!stop_requested_ && WiFi.status() != WL_CONNECTED) {
            if (millis() - begin_ms >= connect_timeout_ms_) {
                Error error("WiFi connection timed out");
                emitError(error);
                started->set_value(tl::make_unexpected(error));
                failPending(error);
                WiFi.disconnect(true);
                running_ = false;
                return;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (stop_requested_) {
            Error error("WiFi stopped while connecting");
            started->set_value(tl::make_unexpected(error));
            failPending(error);
            WiFi.disconnect(true);
            running_ = false;
            return;
        }

        started->set_value(true);

        while (!stop_requested_) {
            Command command;
            {
                std::unique_lock<std::mutex> lock(command_mutex_);
                command_cv_.wait(lock, [this] {
                    return stop_requested_ || !commands_.empty();
                });
                if (stop_requested_) break;
                command = std::move(commands_.front());
                commands_.pop_front();
            }

            auto result = performRequest(command);
            if (!result.has_value()) emitError(result.error());
            command.promise->set_value(std::move(result));
        }

        Error stopped("WiFi stopped");
        failPending(stopped);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        running_ = false;
    }

    Result<WifiResponse> performRequest(const Command& command) {
        if (WiFi.status() != WL_CONNECTED)
            return tl::make_unexpected(Error("WiFi disconnected"));

        const String& url = command.path;
        if (!url.startsWith("http://") && !url.startsWith("https://"))
            return tl::make_unexpected(
                Error("request path must be an absolute URL"));

        HTTPClient http;
        WiFiClientSecure secure_client;
        WiFiClient plain_client;
        secure_client.setInsecure();

        const bool secure = url.startsWith("https://");
        WiFiClient& client = secure
            ? static_cast<WiFiClient&>(secure_client)
            : plain_client;

        if (!http.begin(client, url))
            return tl::make_unexpected(Error("HTTP initialization failed"));

        http.setConnectTimeout(request_timeout_ms_);
        http.setTimeout(request_timeout_ms_);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Connection", "close");

        const int code = command.body.isEmpty()
            ? http.sendRequest(command.method.c_str())
            : http.sendRequest(command.method.c_str(), command.body);
        const String response_body = code >= 0 ? http.getString() : String();
        http.end();

        if (code < 0)
            return tl::make_unexpected(
                Error(HTTPClient::errorToString(code), code));
        return WifiResponse{code, response_body};
    }

    void failPending(const Error& error) {
        std::deque<Command> pending;
        {
            std::lock_guard<std::mutex> lock(command_mutex_);
            pending.swap(commands_);
        }
        for (auto& command : pending)
            command.promise->set_value(tl::make_unexpected(error));
    }

    void emitError(const Error& error) {
        std::vector<ErrorCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            callbacks = error_callbacks_;
        }
        for (const auto& callback : callbacks)
            callback(error);
    }

    String ssid_;
    String password_;
    uint32_t connect_timeout_ms_ = 30000;
    uint16_t request_timeout_ms_ = 10000;

    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::thread worker_;
    std::mutex state_mutex_;

    std::mutex command_mutex_;
    std::condition_variable command_cv_;
    std::deque<Command> commands_;

    std::mutex callback_mutex_;
    std::vector<ErrorCallback> error_callbacks_;
};
