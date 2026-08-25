// wifi.hpp

#pragma once

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <FreeRTOS/Queue.hpp>
#include <FreeRTOS/Task.hpp>
#include <atomic>
#include <cstring>
#include <functional>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>
#include "./result.hpp"

struct WifiResponse {
    int code;
    String body;
};

struct WifiHeader {
    String name;
    String value;
};

using WifiHeaders = std::vector<WifiHeader>;

class Wifi {
private:
    static constexpr size_t MAX_PENDING_REQUESTS = 16;
    static constexpr size_t MAX_HEADERS = 8;
    static constexpr size_t MAX_HEADER_BYTES = 1024;

    enum class CommandType : uint8_t {
        Request,
    };

    struct Command {
        CommandType type;
        char method[16];
        char url[320];
        char body[512];
        uint8_t header_count;
        char headers[MAX_HEADER_BYTES];
    };

    static_assert(std::is_trivially_copyable<Command>::value,
                  "WiFi commands must be safe for a FreeRTOS queue");

    // ESP-IDF defines StackType_t as one byte: this is a 16 KiB stack.
    class WorkerTask final : public FreeRTOS::StaticTask<16384> {
    public:
        explicit WorkerTask(Wifi& owner)
            : FreeRTOS::StaticTask<16384>(tskIDLE_PRIORITY, "wifi"),
              owner_(owner) {}

        void taskFunction() final {
            for (;;) {
                notifyTake(portMAX_DELAY);
                setPriority(1);
                owner_.workerLoop();
                setPriority(tskIDLE_PRIORITY);
                owner_.worker_done_.overwrite(true);
            }
        }

    private:
        Wifi& owner_;
    };

public:
    using ErrorCallback = std::function<void(const Error&)>;
    using ResponseCallback = std::function<void(const WifiResponse&)>;

    Wifi() : worker_task_(*this) {}

    Wifi(String ssid, String password)
        : ssid_(std::move(ssid)),
          password_(std::move(password)),
          worker_task_(*this) {}

    ~Wifi() {
        (void)stop();
    }

    Wifi(const Wifi&) = delete;
    Wifi& operator=(const Wifi&) = delete;

    Wifi& configure(String ssid, String password) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (running_)
            return *this;
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

    Wifi& onResponse(ResponseCallback callback) {
        if (callback) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            response_callbacks_.push_back(std::move(callback));
        }
        return *this;
    }

    Result<bool> start() {
        std::unique_lock<std::mutex> lock(state_mutex_);
        if (running_) return true;
        if (ssid_.isEmpty())
            return tl::make_unexpected(Error("WiFi is not configured"));
        if (!commands_.isValid() || !started_.isValid() ||
            !worker_done_.isValid())
            return tl::make_unexpected(Error("WiFi queue initialization failed"));

        commands_.reset();
        started_.reset();
        worker_done_.reset();
        stop_requested_ = false;
        running_ = true;
        Serial.println("[wifi] starting worker");
        worker_task_.notifyGive();
        lock.unlock();

        auto connected = started_.receive(
            pdMS_TO_TICKS(connect_timeout_ms_ + 5000));
        if (!connected) {
            stop_requested_ = true;
            Error error("WiFi start timed out");
            emitError(error);
            return tl::make_unexpected(error);
        }
        if (!*connected)
            return tl::make_unexpected(Error("WiFi connection failed"));
        return true;
    }

    Result<bool> stop() {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!running_) return true;
            Serial.println("[wifi] stopping worker");
            stop_requested_ = true;
        }

        auto stopped = worker_done_.receive(
            pdMS_TO_TICKS(request_timeout_ms_ + 2000));
        if (!stopped) {
            Error error("WiFi worker did not stop");
            emitError(error);
            return tl::make_unexpected(error);
        }

        commands_.reset();
        return true;
    }

    Result<bool> request(
        const String& method,
        const String& path,
        const String& body,
        const WifiHeaders& headers) {

        if (!running_ || stop_requested_)
            return tl::make_unexpected(Error("WiFi is not running"));

        Command command{};
        if (method.length() >= sizeof(command.method))
            return tl::make_unexpected(Error("HTTP method is too long"));
        if (path.length() >= sizeof(command.url))
            return tl::make_unexpected(Error("request URL is too long"));
        if (body.length() >= sizeof(command.body))
            return tl::make_unexpected(Error("request body is too long"));
        if (headers.size() > MAX_HEADERS)
            return tl::make_unexpected(Error("too many HTTP headers"));

        command.type = CommandType::Request;
        strlcpy(command.method, method.c_str(), sizeof(command.method));
        strlcpy(command.url, path.c_str(), sizeof(command.url));
        strlcpy(command.body, body.c_str(), sizeof(command.body));

        size_t header_offset = 0;
        for (const auto& header : headers) {
            if (header.name.isEmpty())
                return tl::make_unexpected(Error("HTTP header name is empty"));

            const size_t name_size = header.name.length() + 1;
            const size_t value_size = header.value.length() + 1;
            if (header_offset + name_size + value_size >
                sizeof(command.headers))
                return tl::make_unexpected(
                    Error("HTTP headers are too large"));

            memcpy(
                command.headers + header_offset,
                header.name.c_str(),
                name_size);
            header_offset += name_size;
            memcpy(
                command.headers + header_offset,
                header.value.c_str(),
                value_size);
            header_offset += value_size;
            ++command.header_count;
        }

        if (!commands_.sendToBack(command, 0))
            return tl::make_unexpected(Error("WiFi request queue is full"));

        Serial.println(
            "[wifi] queued " + method + " " + path + " (pending: " +
            String(commands_.messagesWaiting()) + ")");
        return true;
    }

    Result<String> ip() const {
        if (WiFi.status() != WL_CONNECTED)
            return tl::make_unexpected(Error("not connected"));
        return WiFi.localIP().toString();
    }

private:
    void workerLoop() {
        Serial.println("[wifi] connecting to " + ssid_);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_.c_str(), password_.c_str());

        const uint32_t begin_ms = millis();
        while (!stop_requested_ && WiFi.status() != WL_CONNECTED) {
            if (millis() - begin_ms >= connect_timeout_ms_) {
                Error error("WiFi connection timed out");
                emitError(error);
                started_.overwrite(false);
                WiFi.disconnect(true);
                running_ = false;
                return;
            }
            FreeRTOS::TaskBase::delay(pdMS_TO_TICKS(100));
        }

        if (stop_requested_) {
            Error error("WiFi stopped while connecting");
            started_.overwrite(false);
            WiFi.disconnect(true);
            running_ = false;
            return;
        }

        Serial.println(
            "[wifi] connected, IP: " + WiFi.localIP().toString());
        started_.overwrite(true);

        while (!stop_requested_) {
            auto command = commands_.receive(pdMS_TO_TICKS(100));
            if (!command) continue;

            auto result = performRequest(*command);
            if (result.has_value())
                emitResponse(result.value());
            else
                emitError(result.error());
        }

        commands_.reset();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        running_ = false;
        Serial.println("[wifi] stopped");
    }

    Result<WifiResponse> performRequest(const Command& command) {
        if (WiFi.status() != WL_CONNECTED)
            return tl::make_unexpected(Error("WiFi disconnected"));

        const String url(command.url);
        Serial.println(
            "[wifi] sending " + String(command.method) + " " + url);
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

        const char* header_cursor = command.headers;
        for (uint8_t index = 0; index < command.header_count; ++index) {
            const String name(header_cursor);
            header_cursor += strlen(header_cursor) + 1;
            const String value(header_cursor);
            header_cursor += strlen(header_cursor) + 1;
            http.addHeader(name, value);
        }

        const String body(command.body);
        const int code = body.isEmpty()
            ? http.sendRequest(command.method)
            : http.sendRequest(command.method, body);
        const String response_body = code >= 0 ? http.getString() : String();
        http.end();

        Serial.println(
            "[wifi] response code=" + String(code) + " bytes=" +
            String(response_body.length()));

        if (code < 0)
            return tl::make_unexpected(
                Error(HTTPClient::errorToString(code), code));
        return WifiResponse{code, response_body};
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

    void emitResponse(const WifiResponse& response) {
        std::vector<ResponseCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            callbacks = response_callbacks_;
        }
        for (const auto& callback : callbacks)
            callback(response);
    }

    String ssid_;
    String password_;
    uint32_t connect_timeout_ms_ = 30000;
    uint16_t request_timeout_ms_ = 10000;

    FreeRTOS::StaticQueue<Command, MAX_PENDING_REQUESTS> commands_;
    FreeRTOS::StaticQueue<bool, 1> started_;
    FreeRTOS::StaticQueue<bool, 1> worker_done_;
    WorkerTask worker_task_;

    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::mutex state_mutex_;

    std::mutex callback_mutex_;
    std::vector<ErrorCallback> error_callbacks_;
    std::vector<ResponseCallback> response_callbacks_;
};
