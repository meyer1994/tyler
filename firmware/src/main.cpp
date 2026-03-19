#include <Arduino.h>
#include <HTTPClient.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <limits>

#include "./future.hpp"

#define GPS_RX_PIN 22
#define GPS_TX_PIN -1
#define GPS_BAUD   9600

#ifndef WIFI_SSID
#error "WIFI_SSID is not defined"
#endif

#ifndef WIFI_PASS
#error "WIFI_PASS is not defined"
#endif

#ifndef SERVER_URL
#error "SERVER_URL is not defined"
#endif

struct GpsFix {
    double   lat;
    double   lng;
    uint32_t sats;
};

// ── Gps ──────────────────────────────────────────────────────────────────────

class Gps {
    freertos::promise<bool>  _conn_promise;
    freertos::future<bool>   _conn_future;

public:
    Gps() : _conn_future(_conn_promise.get_future()) {}

    freertos::future<bool>& open() {
        _running = true;
        _serial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        xTaskCreate(_entry, "gps", 8192, this, 1, &_task);
        return _conn_future;
    }

    freertos::future<bool>& isOpen() { return _conn_future; }

    // Returns an error immediately if not connected —
    // no deferred broken_promise, no zombie future.
    tl::expected<freertos::future<GpsFix>, freertos::future_errc> fix() {
        if (!_conn_future.is_ready())
            return tl::unexpected(freertos::future_errc::broken_promise);

        // Fresh promise/future pair for each fix attempt.
        _fix_promise = freertos::promise<GpsFix>{};
        _fix_future  = _fix_promise.get_future();
        return _fix_future;
    }

    freertos::future<bool> close() {
        _stop_promise = freertos::promise<bool>{};
        _running      = false;
        return _stop_promise.get_future();
    }

private:
    HardwareSerial             _serial{2};
    TaskHandle_t               _task{nullptr};
    TinyGPSPlus                _gps;
    freertos::promise<GpsFix>  _fix_promise;
    freertos::future<GpsFix>   _fix_future{_fix_promise.get_future()};
    freertos::promise<bool>    _stop_promise;
    std::atomic<bool>          _running{false};

    static void _entry(void* arg) {
        static_cast<Gps*>(arg)->_run();
        vTaskDelete(nullptr);
    }

    void _run() {
        while (_running.load()) {
            while (_serial.available()) _gps.encode(_serial.read());

            if (_gps.location.isValid() && _gps.location.isUpdated()) {
                GpsFix f = {
                    .lat  = _gps.location.lat(),
                    .lng  = _gps.location.lng(),
                    .sats = _gps.satellites.value()
                };
                _conn_promise.set_value(true);
                _fix_promise.set_value(f);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        _serial.end();
        _stop_promise.set_value(true);
    }
};

// ── Wifi ──────────────────────────────────────────────────────────────────────

class Wifi {
    freertos::promise<bool>  _conn_promise;
    freertos::future<bool>   _conn_future;

public:
    Wifi() : _conn_future(_conn_promise.get_future()) {}

    freertos::future<bool>& open(const char* ssid, const char* pass) {
        WiFi.mode(WIFI_STA);
        _handler = WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t) {
            _onEvent(event);
        });
        WiFi.begin(ssid, pass);
        return _conn_future;
    }

    freertos::future<bool>& isOpen() { return _conn_future; }

    tl::expected<freertos::future<bool>, freertos::future_errc> post(const GpsFix& fix) {
        if (!_conn_future.is_ready())
            return tl::unexpected(freertos::future_errc::broken_promise);

        // Fresh promise/future pair for each post attempt.
        _post_promise = freertos::promise<bool>{};
        _post_future  = _post_promise.get_future();
        // Dispatch HTTP POST to a task so setup() can await the future.
        auto* ctx = new PostCtx{this, fix};
        xTaskCreate(_post_entry, "http_post", 8192, ctx, 1, nullptr);
        return _post_future;
    }

    freertos::future<bool> close() {
        _removeHandler();
        _disc_promise = freertos::promise<bool>{};
        _handler = WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t) {
            if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                _removeHandler();
                _disc_promise.set_value(true);
            }
        });
        WiFi.disconnect(true);
        return _disc_promise.get_future();
    }

private:
    struct PostCtx { Wifi* self; GpsFix fix; };

    wifi_event_id_t         _handler{WIFI_EVENT_MAX};
    freertos::promise<bool> _disc_promise;
    freertos::promise<bool> _post_promise;
    freertos::future<bool>  _post_future{_post_promise.get_future()};

    void _removeHandler() {
        if (_handler) {
            WiFi.removeEvent(_handler);
            _handler = WIFI_EVENT_MAX;
        }
    }

    void _onEvent(WiFiEvent_t event) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                _conn_promise.set_value(true);
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                _conn_promise.set_error(freertos::future_errc::broken_promise);
                break;
            default:
                break;
        }
    }

    static void _post_entry(void* arg) {
        auto* ctx = static_cast<PostCtx*>(arg);
        ctx->self->_post(ctx->fix);
        delete ctx;
        vTaskDelete(nullptr);
    }

    void _post(const GpsFix& fix) {
        HTTPClient http;
        http.begin(SERVER_URL);
        http.addHeader("Content-Type", "application/json");

        char body[128];
        snprintf(body, sizeof(body),
            "{\"lat\":%.6f,\"lng\":%.6f,\"sats\":%u}",
            fix.lat, fix.lng, fix.sats);

        int code = http.POST(body);
        http.end();

        _post_promise.set_value(code >= 200 && code < 300);
    }
};

// ── setup ─────────────────────────────────────────────────────────────────────

void deep(uint32_t seconds, Gps& gps, Wifi& wifi) {
    (void)gps.close().get_for_ms(5000);
    (void)wifi.close().get_for_ms(5000);
    esp_sleep_enable_timer_wakeup(1ULL * 1'000'000ULL);
    // esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1'000'000ULL);
    Serial.printf("Sleeping for %u s\r\n", seconds);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up...");

    Gps  gps;
    Wifi wifi;

    gps.open();
    wifi.open(WIFI_SSID, WIFI_PASS);

    if (gps.isOpen().wait_for_ms(90'000) != freertos::future<bool>::status::ready) {
        Serial.println("GPS connection failed");
        return deep(300, gps, wifi);
    }
    Serial.println("GPS fix acquired");

    if (wifi.isOpen().wait_for_ms(0) != freertos::future<bool>::status::ready) {
        Serial.println("WiFi connection failed");
        return deep(300, gps, wifi);
    }
    Serial.println("WiFi connected");

    auto fixFut = gps.fix();
    if (!fixFut) {
        Serial.println("GPS not connected");
        return deep(60, gps, wifi);
    }
    if (fixFut->wait_for_ms(5'000) != freertos::future<GpsFix>::status::ready) {
        Serial.println("GPS fix timed out");
        return deep(60, gps, wifi);
    }
    const GpsFix fix = *fixFut->get();
    Serial.printf("GPS: lat=%.6f, lng=%.6f, sats=%u\r\n", fix.lat, fix.lng, fix.sats);

    auto postFut = wifi.post(fix);
    if (!postFut) {
        Serial.println("WiFi not connected");
        return deep(60, gps, wifi);
    }
    if (postFut->wait_for_ms(15'000) != freertos::future<bool>::status::ready) {
        Serial.println("WiFi post timed out");
        return deep(60, gps, wifi);
    }
    const bool ok = *postFut->get();
    Serial.printf("Post %s\r\n", ok ? "succeeded" : "failed");

    return deep(300, gps, wifi);
}

void loop() {
    vTaskDelete(NULL);
}
