#include <Arduino.h>
#include <esp_task_wdt.h>
#if defined(TRACKOWS_GPS_SIM7000G)
#include <Ticker.h>
#endif

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
const WifiHeaders event_headers{
    {"Content-Type", "application/json"},
    {"Connection", "close"},
};

#if defined(TRACKOWS_GPS_SIM7000G)
constexpr uint8_t RESPONSE_LED_PIN = 12;
constexpr uint32_t RESPONSE_LED_DURATION_MS = 100;
Ticker response_led_timer;
#endif

void stopServices() {
    (void)gps.stop();
    (void)wifi.stop();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.println("[main] starting up...");
    Serial.println("[main] WiFi SSID: " + String(WIFI_SSID));
    Serial.println("[main] WiFi password: " + String(WIFI_PASS));

    delay(2000);
    esp_task_wdt_deinit();

#if defined(TRACKOWS_GPS_SIM7000G)
    pinMode(RESPONSE_LED_PIN, OUTPUT);
    digitalWrite(RESPONSE_LED_PIN, HIGH);  // Active-low LED: start off.
#endif

    // Register every hook before starting either service.
    wifi.onError([](const Error& error) {
        Serial.println("[wifi] error: " + error.message);
    });

#if defined(TRACKOWS_GPS_SIM7000G)
    wifi.onResponse([](const WifiResponse& response) {
        if (response.code < 200 || response.code >= 300) return;

        digitalWrite(RESPONSE_LED_PIN, LOW);
        response_led_timer.once_ms(RESPONSE_LED_DURATION_MS, [] {
            digitalWrite(RESPONSE_LED_PIN, HIGH);
        });
    });
#endif

    gps.onError([](const Error& error) {
        Serial.println("[gps] error: " + error.message);
    });

    gps.onState([](GPSState previous, GPSState current) {
        Serial.println(
            "[gps] state: " + String(previous.getName()) + " -> " +
            current.getName());
    });

    gps.onFixStateChange(
        [](GPSFixState previous, GPSFixState current) {
            Serial.println(
                "[gps] fix state: " + String(previous.getName()) +
                " -> " + current.getName());
        });

    gps.onFix([](const GPSFix& fix) {
        const String body = fix.toJson();
        Serial.println("[gps] enqueue fix: " + body);

        // request() only queues the operation. The Wi-Fi worker performs HTTP,
        // so this callback returns immediately and GPS parsing remains active.
        auto request =
            wifi.request("POST", SERVER_URL, body, event_headers);

        if (!request.has_value())
            Serial.println("[main] request rejected: " +
                           request.error().message);
    });

    auto wifi_result = wifi.start();
    if (!wifi_result.has_value()) {
        Serial.println("[main] WiFi start failed: " +
                       wifi_result.error().message);
        stopServices();
        return;
    }

    auto gps_result = gps.start();
    if (!gps_result.has_value()) {
        Serial.println("[main] GPS start failed: " +
                       gps_result.error().message);
        stopServices();
        return;
    }

    auto ip = wifi.ip();
    if (ip.has_value())
        Serial.println("[main] WiFi IP: " + ip.value());

    Serial.println("[main] streaming every GPS fix");
}

void loop() {
    // GPS and Wi-Fi run in dedicated FreeRTOS tasks.
    delay(1000);
}
