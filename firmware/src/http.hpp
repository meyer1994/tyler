#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FreeRTOS/Queue.hpp>
#include <FreeRTOS/Task.hpp>
#include <atomic>
#include <mutex>

#include "./events.hpp"
#include "./network.hpp"
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
    HttpService(Network& network, String server_url)
        : network_(network),
          server_url_(std::move(server_url)),
          worker_(*this) {}

    ~HttpService() { (void)stop(); }

    HttpService(const HttpService&) = delete;
    HttpService& operator=(const HttpService&) = delete;

    EventEmitter& events() { return events_; }

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

    void _poll() {
        while (!stop_requested_) {
            auto fix = requests_.receive(pdMS_TO_TICKS(100));
            if (!fix) continue;
            performRequest(*fix);
        }
    }

    HttpResponse parseResponse(int code, const String& body) {
        HttpResponse response{code, false, false};
        if (code != 200) return response;

        JsonDocument document;
        const auto error = deserializeJson(document, body);
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

    void performRequest(const GPSFix& fix) {
        const String mac = network_.macAddress();
        JsonDocument document;
        fix.toJson(document);
        document["mac"] = mac;

        String body;
        serializeJson(document, body);

        Serial.println("[http] POST " + server_url_ + " mac=" + mac);
        auto result = network_.post(server_url_, "application/json", body);
        if (!result) {
            emitError(result.error());
            return;
        }

        Serial.println(
            "[http] response code=" + String(result->code) +
            " body=" + result->body);
        const HttpResponse response = parseResponse(result->code, result->body);
        Serial.println(
            "[http] response ok=" + String(response.ok ? "true" : "false") +
            " intersects=" +
            String(response.intersects ? "true" : "false"));
        events_.emit(HttpEvent::Response, response);
    }

    void emitError(const Error& error) {
        events_.emit(HttpEvent::Error, eventError(error));
    }

    EventEmitter events_;
    Network& network_;
    String server_url_;
    uint16_t request_timeout_ms_ = 10000;

    FreeRTOS::StaticQueue<GPSFix, MAX_PENDING_REQUESTS> requests_;
    FreeRTOS::StaticQueue<bool, 1> worker_done_;
    WorkerTask worker_;

    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::mutex state_mutex_;
};
