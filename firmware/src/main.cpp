#include <Arduino.h>
#include <esp_task_wdt.h>
#include <chrono>
#include <future>

#include "./result.hpp"
#include "./gps.hpp"
#include "./wifi.hpp"

#ifndef WIFI_SSID
#error "WIFI_SSID is not defined"
#endif

#ifndef WIFI_PASS
#error "WIFI_PASS is not defined"
#endif

#ifndef SERVER_URL
#error "SERVER_URL is not defined"
#endif

namespace {

GPS gps;
Wifi wifi(WIFI_SSID, WIFI_PASS);

template <typename T>
bool waitReady(std::future<T>& future, uint32_t timeout_ms) {
    return future.wait_for(std::chrono::milliseconds(timeout_ms)) ==
           std::future_status::ready;
}

void stopServices() {
    auto gps_stopped = gps.stop();
    auto wifi_stopped = wifi.stop();
    gps_stopped.wait();
    wifi_stopped.wait();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.println("[main] starting up...");
    Serial.println("[main] WiFi SSID: " + String(WIFI_SSID));
    Serial.println("[main] WiFi password: " + String(WIFI_PASS));

    delay(2000);
    esp_task_wdt_deinit();

    // Register every hook before starting either service.
    wifi.onError([](const Error& error) {
        Serial.println("[wifi] error: " + error.message);
    });

    gps.onError([](const Error& error) {
        Serial.println("[gps] error: " + error.message);
    });

    gps.onFix([](const GPSFix& fix) {
        const String body = fix.toJson();
        Serial.println("[gps] enqueue fix: " + body);

        // request() only queues the operation. The Wi-Fi worker performs HTTP,
        // so this callback returns immediately and GPS parsing remains active.
        auto request = wifi.request("POST", SERVER_URL, body);

        // A ready future here means queueing failed immediately (for example,
        // because Wi-Fi is stopping). Normal queued requests finish later and
        // report failures through wifi.onError().
        if (request.wait_for(std::chrono::milliseconds(0)) ==
            std::future_status::ready) {
            auto result = request.get();
            if (!result.has_value())
                Serial.println("[main] request rejected: " +
                               result.error().message);
        }
    });

    // Wi-Fi is marked running before GPS can emit its first fix, allowing fixes
    // received while Wi-Fi connects to wait in the request queue.
    auto wifi_started = wifi.start();
    auto gps_started = gps.start();

    if (!waitReady(gps_started, 5000)) {
        Serial.println("[main] GPS start timed out");
        stopServices();
        return;
    }

    auto gps_result = gps_started.get();
    if (!gps_result.has_value()) {
        Serial.println("[main] GPS start failed: " +
                       gps_result.error().message);
        stopServices();
        return;
    }

    if (!waitReady(wifi_started, 35000)) {
        Serial.println("[main] WiFi start timed out");
        stopServices();
        return;
    }

    auto wifi_result = wifi_started.get();
    if (!wifi_result.has_value()) {
        Serial.println("[main] WiFi start failed: " +
                       wifi_result.error().message);
        stopServices();
        return;
    }

    auto ip = wifi.ip();
    if (ip.has_value())
        Serial.println("[main] WiFi IP: " + ip.value());

    Serial.println("[main] streaming every GPS fix");
}

void loop() {
    // GPS and Wi-Fi run on their own threads. Arduino's loop task has no work.
    delay(1000);
}
