#pragma once

#include <Arduino.h>
#include <esp_http_client.h>

// Arduino's esp_crt_bundle.h shadows the ESP-IDF header with a separate
// bundle implementation that must be initialized manually. esp_http_client
// uses the certificate bundle embedded in the ESP-IDF SDK instead.
extern "C" esp_err_t esp_crt_bundle_attach(void* config);
#include <FreeRTOS/Queue.hpp>
#include <FreeRTOS/Task.hpp>
#include <atomic>
#include <mutex>

#include "./events.hpp"
#include "./result.hpp"

class HttpService {
private:
    static constexpr size_t WORKER_COUNT = 2;
    static constexpr size_t MAX_PENDING_REQUESTS = 16;

    class WorkerTask final : public FreeRTOS::StaticTask<12288> {
    public:
        WorkerTask(
            HttpService& owner,
            uint8_t index,
            const char* name)
            : FreeRTOS::StaticTask<12288>(tskIDLE_PRIORITY, name),
              owner_(owner),
              index_(index) {}

        void taskFunction() final {
            for (;;) {
                notifyTake(portMAX_DELAY);
                setPriority(1);
                owner_.workerLoop(index_);
                setPriority(tskIDLE_PRIORITY);
                owner_.worker_done_.sendToBack(true, portMAX_DELAY);
            }
        }

    private:
        HttpService& owner_;
        uint8_t index_;
    };

public:
    HttpService(HttpEmitter& events, String server_url)
        : events_(events),
          server_url_(std::move(server_url)),
          workers_{
              WorkerTask(*this, 0, "http_0"),
              WorkerTask(*this, 1, "http_1"),
          } {}

    ~HttpService() { (void)stop(); }

    HttpService(const HttpService&) = delete;
    HttpService& operator=(const HttpService&) = delete;

    Result<bool> start() {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (running_) return true;
        if (server_url_.isEmpty())
            return tl::make_unexpected(Error("HTTP server URL is empty"));
        if (!requests_.isValid() || !worker_done_.isValid())
            return tl::make_unexpected(Error("HTTP queue initialization failed"));

        (void)events_.on<HttpEvent::PostLocation>(
            &HttpService::enqueueLocation,
            this);

        requests_.reset();
        worker_done_.reset();
        stop_requested_ = false;
        running_ = true;
        for (auto& worker : workers_) worker.notifyGive();
        return true;
    }

    Result<bool> stop() {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!running_) return true;
            stop_requested_ = true;
        }

        bool clean = true;
        for (size_t index = 0; index < WORKER_COUNT; ++index) {
            if (!worker_done_.receive(pdMS_TO_TICKS(request_timeout_ms_ + 2000)))
                clean = false;
        }

        requests_.reset();
        running_ = false;
        if (!clean) {
            const Error error("HTTP worker did not stop");
            emitError(error);
            return tl::make_unexpected(error);
        }
        return true;
    }

private:
    static void enqueueLocation(
        const HttpPostLocation& location,
        void* context) {
        static_cast<HttpService*>(context)->enqueue(location.fix);
    }

    void enqueue(const GPSFix& fix) {
        if (!running_ || stop_requested_) {
            emitError(Error("HTTP service is not running"));
            return;
        }
        if (!requests_.sendToBack(fix, 0)) {
            emitError(Error("HTTP request queue is full"));
            return;
        }
        Serial.println(
            "[http] location queued (pending: " +
            String(requests_.messagesWaiting()) + ")");
    }

    void workerLoop(uint8_t worker_index) {
        esp_http_client_config_t config{};
        config.url = server_url_.c_str();
        config.method = HTTP_METHOD_POST;
        config.timeout_ms = request_timeout_ms_;
        config.keep_alive_enable = true;
        config.crt_bundle_attach = esp_crt_bundle_attach;

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (!client) {
            emitError(Error("HTTP client initialization failed"));
            return;
        }

        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "Connection", "keep-alive");

        while (!stop_requested_) {
            auto fix = requests_.receive(pdMS_TO_TICKS(100));
            if (!fix) continue;
            performRequest(client, *fix, worker_index);
        }

        esp_http_client_cleanup(client);
    }

    void performRequest(
        esp_http_client_handle_t client,
        const GPSFix& fix,
        uint8_t worker_index) {
        const String body = fix.toJson();
        esp_http_client_set_post_field(client, body.c_str(), body.length());

        Serial.println(
            "[http:" + String(worker_index) + "] POST " + server_url_);
        const esp_err_t result = esp_http_client_perform(client);
        if (result != ESP_OK) {
            emitError(Error(
                "HTTP request failed: " + String(esp_err_to_name(result)),
                result));
            return;
        }

        const int code = esp_http_client_get_status_code(client);
        Serial.println(
            "[http:" + String(worker_index) + "] response code=" +
            String(code));
        (void)events_.emit<HttpEvent::Response>({code, worker_index});
    }

    void emitError(const Error& error) {
        (void)events_.emit<HttpEvent::Error>(eventError(error));
    }

    HttpEmitter& events_;
    String server_url_;
    uint16_t request_timeout_ms_ = 10000;

    FreeRTOS::StaticQueue<GPSFix, MAX_PENDING_REQUESTS> requests_;
    FreeRTOS::StaticQueue<bool, WORKER_COUNT> worker_done_;
    WorkerTask workers_[WORKER_COUNT];

    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::mutex state_mutex_;
};
