#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_task_wdt.h>

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

void run(GPS& gps, Cellular& cell) {
    auto futGpsOpen = gps.open();
    auto futCellOpen = cell.open(CELLULAR_APN);

    auto resGpsOpen = futGpsOpen.wait(60000);
    if (resGpsOpen.has_value()) {
        Serial.println("[main] GPS opened");
    } else {
        Serial.println("[main] GPS failed: " + resGpsOpen.error().message);
        return;
    }

    auto resCellOpen = futCellOpen.wait(90000);
    if (resCellOpen.has_value()) {
        Serial.println("[main] cellular connected");
    } else {
        Serial.println("[main] cellular failed: " + resCellOpen.error().message);
        return;
    }

    auto ip = cell.ip();
    if (ip.has_value()) {
        Serial.println("[main] IP: " + ip.value());
    } else {
        Serial.println("[main] IP failed: " + ip.error().message);
        return;
    }

    auto futGpsFix = gps.fix();
    auto resGpsFix = futGpsFix.wait(10000);
    if (!resGpsFix.has_value()) {
        Serial.println("[main] GPS fix failed: " + resGpsFix.error().message);
        return;
    }

    auto fix = resGpsFix.value();
    char timestamp[21];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ",
        fix.year, fix.month, fix.day, fix.hour, fix.minute, fix.second);

    String body = "{";
    body += "\"lat\":" + String(fix.lat, 6) + ",";
    body += "\"lon\":" + String(fix.lng, 6) + ",";
    body += "\"alt\":" + String(fix.altitude, 2) + ",";
    body += "\"speed\":" + String(fix.speed, 1) + ",";
    body += "\"heading\":" + String(fix.heading, 1) + ",";
    body += "\"hdop\":" + String(fix.hdop, 1) + ",";
    body += "\"sat\":" + String(fix.satellites) + ",";
    body += "\"ts\":\"" + String(timestamp) + "\"";
    body += "}";
    Serial.println("[main] body: " + body);

    auto res = cell.post(SERVER_HOST, SERVER_PORT, SERVER_PATH, body).wait(10000);
    if (res.has_value()) {
        Serial.println("[main] POST " + String(res.value()));
    } else {
        Serial.println("[main] POST failed: " + res.error().message);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("[main] starting up...");
    esp_task_wdt_deinit();

    GPS gps;
    Cellular cell;

    run(gps, cell);

    Serial.println("[main] sleeping for 60s...");
    esp_sleep_enable_timer_wakeup(60ULL * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
    vTaskDelete(NULL);
}
