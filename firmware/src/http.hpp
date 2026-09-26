#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
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
    static constexpr size_t MAX_PENDING_REQUESTS = 16;

    class WorkerTask final : public FreeRTOS::StaticTask<12288> {
    public:
        explicit WorkerTask(HttpService& owner)
            : FreeRTOS::StaticTask<12288>(tskIDLE_PRIORITY, "http"),
              owner_(owner) {}

        void taskFunction() final {
            for (;;) {
                notifyTake(portMAX_DELAY);
                setPriority(1);
                owner_._poll();
                setPriority(tskIDLE_PRIORITY);
                owner_.worker_done_.sendToBack(true, portMAX_DELAY);
            }
        }

    private:
        HttpService& owner_;
    };

public:
    HttpService(EventEmitter& events, String server_url)
        : events_(events),
          server_url_(std::move(server_url)),
          worker_(*this) {}

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

        events_.on(HttpEvent::PostLocation, [this](void* data) {
            enqueue(static_cast<HttpPostLocation*>(data)->fix);
        });

        requests_.reset();
        worker_done_.reset();
        stop_requested_ = false;
        running_ = true;
        worker_.notifyGive();
        return true;
    }

    Result<bool> stop() {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!running_) return true;
            stop_requested_ = true;
        }

        const bool clean = worker_done_.receive(
            pdMS_TO_TICKS(request_timeout_ms_ + 2000)).value_or(false);

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

    static esp_err_t captureResponse(esp_http_client_event_t* event) {
        if (event->event_id != HTTP_EVENT_ON_DATA ||
            !event->user_data ||
            !event->data ||
            event->data_len <= 0)
            return ESP_OK;

        auto* service = static_cast<HttpService*>(event->user_data);
        service->response_body_.concat(
            static_cast<const char*>(event->data),
            event->data_len);
        return ESP_OK;
    }

    void _poll() {
        esp_http_client_config_t config{};
        config.url = server_url_.c_str();
        config.method = HTTP_METHOD_POST;
        config.timeout_ms = request_timeout_ms_;
        config.keep_alive_enable = true;
        config.crt_bundle_attach = esp_crt_bundle_attach;
        config.event_handler = &captureResponse;
        config.user_data = this;

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
            performRequest(client, *fix);
        }

        esp_http_client_cleanup(client);
    }

    HttpResponse parseResponse(int code) {
        HttpResponse response{code, false, false};
        if (code != 200) return response;

        JsonDocument document;
        const auto error = deserializeJson(document, response_body_);
        if (error) {
            emitError(Error(
                "HTTP response JSON parse failed: " +
                String(error.c_str())));
            return response;
        }

        response.ok = document["ok"] | false;
        response.intersects = document["intersects"] | false;
        return response;
    }

    void performRequest(
        esp_http_client_handle_t client,
        const GPSFix& fix) {
        const String body = fix.toJson();
        esp_http_client_set_post_field(client, body.c_str(), body.length());

        Serial.println("[http] POST " + server_url_);
        response_body_ = "";
        const esp_err_t result = esp_http_client_perform(client);
        if (result != ESP_OK) {
            emitError(Error(
                "HTTP request failed: " + String(esp_err_to_name(result)),
                result));
            return;
        }

        const int code = esp_http_client_get_status_code(client);
        Serial.println(
            "[http] response code=" + String(code) +
            " body=" + response_body_);
        const HttpResponse response = parseResponse(code);
        Serial.println(
            "[http] response ok=" + String(response.ok ? "true" : "false") +
            " intersects=" +
            String(response.intersects ? "true" : "false"));
        events_.emit(HttpEvent::Response, response);
    }

    void emitError(const Error& error) {
        events_.emit(HttpEvent::Error, eventError(error));
    }

    EventEmitter& events_;
    String server_url_;
    uint16_t request_timeout_ms_ = 10000;
    String response_body_;

    FreeRTOS::StaticQueue<GPSFix, MAX_PENDING_REQUESTS> requests_;
    FreeRTOS::StaticQueue<bool, 1> worker_done_;
    WorkerTask worker_;

    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::mutex state_mutex_;
};
