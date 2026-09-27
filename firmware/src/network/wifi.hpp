#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_http_client.h>
#include <ctime>
#include <mutex>

#include "../network.hpp"

// Arduino's esp_crt_bundle.h shadows the ESP-IDF header with a separate
// bundle implementation that must be initialized manually. esp_http_client
// uses the certificate bundle embedded in the ESP-IDF SDK instead.
extern "C" esp_err_t esp_crt_bundle_attach(void* config);

class WifiNetwork final : public Network {
public:
    WifiNetwork() = default;
    ~WifiNetwork() override { stop(); }

    WifiNetwork(const WifiNetwork&) = delete;
    WifiNetwork& operator=(const WifiNetwork&) = delete;

    Result<bool> init(const String& ssid, const String& password) {
        if (ssid.isEmpty())
            return tl::make_unexpected(Error("WiFi is not configured"));
        if (connected()) return true;

        Serial.println("[wifi] connecting to " + ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());

        const uint32_t begin_ms = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - begin_ms >= CONNECT_TIMEOUT_MS) {
                WiFi.disconnect(true);
                return tl::make_unexpected(Error("WiFi connection timed out"));
            }
            delay(100);
        }
        Serial.println("[wifi] connected, IP: " + WiFi.localIP().toString());

        configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
        const uint32_t time_begin_ms = millis();
        while (time(nullptr) < MINIMUM_VALID_TIME) {
            if (millis() - time_begin_ms >= TIME_SYNC_TIMEOUT_MS) {
                WiFi.disconnect(true);
                return tl::make_unexpected(
                    Error("network time synchronization timed out"));
            }
            delay(100);
        }
        return true;
    }

    void stop() {
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (client_) {
            esp_http_client_cleanup(client_);
            client_ = nullptr;
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }

    bool connected() const override {
        return WiFi.status() == WL_CONNECTED;
    }

    String macAddress() const override {
        const uint64_t mac = ESP.getEfuseMac();
        char address[18];
        snprintf(
            address,
            sizeof(address),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            static_cast<uint8_t>(mac),
            static_cast<uint8_t>(mac >> 8),
            static_cast<uint8_t>(mac >> 16),
            static_cast<uint8_t>(mac >> 24),
            static_cast<uint8_t>(mac >> 32),
            static_cast<uint8_t>(mac >> 40));
        return address;
    }

    Result<String> ip() const override {
        if (!connected())
            return tl::make_unexpected(Error("WiFi is not connected"));
        return WiFi.localIP().toString();
    }

    Result<NetworkResponse> post(
        const String& url,
        const char* content_type,
        const String& body) override {
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (!connected())
            return tl::make_unexpected(Error("WiFi is not connected"));

        auto client = clientFor(url);
        if (!client) return tl::make_unexpected(client.error());

        esp_http_client_set_method(*client, HTTP_METHOD_POST);
        esp_http_client_set_header(*client, "Content-Type", content_type);
        esp_http_client_set_post_field(*client, body.c_str(), body.length());

        response_body_ = "";
        const esp_err_t result = esp_http_client_perform(*client);
        if (result != ESP_OK)
            return tl::make_unexpected(Error(
                "HTTP request failed: " + String(esp_err_to_name(result)),
                result));

        return NetworkResponse{
            esp_http_client_get_status_code(*client),
            response_body_,
        };
    }

private:
    Result<esp_http_client_handle_t> clientFor(const String& url) {
        if (client_) {
            if (url != client_url_ &&
                esp_http_client_set_url(client_, url.c_str()) != ESP_OK)
                return tl::make_unexpected(Error("invalid HTTP URL"));
            client_url_ = url;
            return client_;
        }

        esp_http_client_config_t config{};
        config.url = url.c_str();
        config.method = HTTP_METHOD_POST;
        config.timeout_ms = REQUEST_TIMEOUT_MS;
        config.keep_alive_enable = true;
        config.crt_bundle_attach = esp_crt_bundle_attach;
        config.event_handler = &captureResponse;
        config.user_data = this;

        client_ = esp_http_client_init(&config);
        if (!client_)
            return tl::make_unexpected(
                Error("HTTP client initialization failed"));
        esp_http_client_set_header(client_, "Connection", "keep-alive");
        client_url_ = url;
        return client_;
    }

    static esp_err_t captureResponse(esp_http_client_event_t* event) {
        if (event->event_id != HTTP_EVENT_ON_DATA ||
            !event->user_data ||
            !event->data ||
            event->data_len <= 0)
            return ESP_OK;

        auto* network = static_cast<WifiNetwork*>(event->user_data);
        network->response_body_.concat(
            static_cast<const char*>(event->data),
            event->data_len);
        return ESP_OK;
    }

    static constexpr time_t MINIMUM_VALID_TIME = 1704067200;  // 2024-01-01
    static constexpr uint32_t CONNECT_TIMEOUT_MS = 30000;
    static constexpr uint32_t TIME_SYNC_TIMEOUT_MS = 15000;
    static constexpr int REQUEST_TIMEOUT_MS = 10000;

    esp_http_client_handle_t client_ = nullptr;
    String client_url_;
    String response_body_;
    std::mutex client_mutex_;
};
