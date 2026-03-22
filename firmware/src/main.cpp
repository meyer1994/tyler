#include <Arduino.h>
#include <esp_sleep.h>

#include "./future.hpp"
#include "./gps.hpp"
#include "./cellular.hpp"

#ifndef SERVER_HOST
#error "SERVER_HOST is not defined"
#endif

#ifndef SERVER_PORT
#error "SERVER_PORT is not defined"
#endif

#ifndef SERVER_PATH
#error "SERVER_PATH is not defined"
#endif

#ifndef CELLULAR_APN
#error "CELLULAR_APN is not defined"
#endif

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up...");

    GPS gps;
    Cellular cell;

    auto futGpsOpen = gps.open();
    auto futCellOpen = cell.open(CELLULAR_APN);

    auto resGpsOpen = futGpsOpen.wait(60000);
    if (resGpsOpen.has_value()) {
        Serial.println("GPS opened");
    } else {
        Serial.println("GPS failed: " + resGpsOpen.error().message);
        return;
    }

    auto resCellOpen = futCellOpen.wait(90000);
    if (resCellOpen.has_value()) {
        Serial.println("Cellular connected!");
    } else {
        Serial.println("Cellular failed: " + resCellOpen.error().message);
        return;
    }

    auto ip = cell.ip();
    if (ip.has_value()) {
        Serial.println("IP: " + ip.value());
    } else {
        Serial.println("IP failed: " + ip.error().message);
        return;
    }

    auto futGpsFix = gps.fix();
    auto resGpsFix = futGpsFix.wait(10000);
    if (!resGpsFix.has_value()) {
        Serial.println("GPS fix failed: " + resGpsFix.error().message);
        return;
    }

    auto fix = resGpsFix.value();
    Serial.println("Lat: " + String(fix.lat, 6) + " Lng: " + String(fix.lng, 6));

    String body = "{";
    body += "\"lat\":" + String(fix.lat, 6) + ",";
    body += "\"lon\":" + String(fix.lng, 6) + ",";
    body += "\"alt\":" + String(fix.altitude, 2) + ",";
    body += "\"sat\":" + String(fix.satellites);
    body += "}";
    Serial.println("Body: " + body);
    
    auto res = cell.post(SERVER_HOST, SERVER_PORT, SERVER_PATH, body).wait(10000);
    if (res.has_value()) {
        Serial.println("POST " + String(res.value()));
    } else {
        Serial.println("POST failed: " + res.error().message);
    }
}

void loop() {
    vTaskDelete(NULL);
}
