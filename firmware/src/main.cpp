#include <Arduino.h>
#include <esp_sleep.h>

#include "./future.hpp"
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

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up...");

    GPS gps;
    Wifi wifi;

    auto futGpsOpen = gps.open();
    auto futWifiOpen = wifi.open(WIFI_SSID, WIFI_PASS);

    auto resGpsOpen = futGpsOpen.wait(60000);
    if (!resGpsOpen.has_value()) {
        Serial.println("Failed to open GPS");
        Serial.println(resGpsOpen.error().message);
        return;
    } else {
        Serial.println("GPS opened!");
    }

    // wifi should already be connected at this point
    auto resWifiOpen = futWifiOpen.wait(5000);
    if (!resWifiOpen.has_value()) {
        Serial.println("Failed to connect to WiFi");
        Serial.println(resWifiOpen.error().message);
        return;
    } else {
        Serial.println("WiFi connected!");
    }

    auto ip = wifi.ip();
    if (ip.has_value()) {
        Serial.println("IP address: " + ip.value());
    } else {
        Serial.println("Failed to get IP address");
        Serial.println(ip.error().message);
        return;
    }

    auto futGpsFix = gps.fix();
    auto resGpsFix = futGpsFix.wait(10000);
    if (!resGpsFix.has_value()) {
        Serial.println("Failed to get GPS fix");
        Serial.println(resGpsFix.error().message);
        return;
    }

    auto fix = resGpsFix.value();
    Serial.println("GPS fix acquired!");
    Serial.println("Lat: " + String(fix.lat, 6) + " Lng: " + String(fix.lng, 6));
    Serial.println("Altitude: " + String(fix.altitude, 2) + "m");
    Serial.println("Satellites: " + String(fix.satellites));

    String body = "{";
    body += "\"lat\":" + String(fix.lat, 6) + ",";
    body += "\"lon\":" + String(fix.lng, 6) + ",";
    body += "\"alt\":" + String(fix.altitude, 2) + ",";
    body += "\"sat\":" + String(fix.satellites);
    body += "}";
    Serial.println("Body: " + body);

    Serial.println("POST " + String(SERVER_URL));
    auto res = wifi.post(SERVER_URL, body).wait(10000);
    if (res.has_value()) {
        Serial.println("POST " + String(res.value()));
    } else {
        Serial.println("POST failed: " + res.error().message);
    }
}

void loop() {
    vTaskDelete(NULL);
}
