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

    auto resGpsOpen = futGpsOpen.wait(90000);
    if (resGpsOpen.has_value()) {
        Serial.println("[main] GPS opened");
    } else {
        Serial.println("[main] GPS failed: " + resGpsOpen.error().message);
        return;
    }

    auto resCellOpen = futCellOpen.wait(60000);
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
    String body = fix.toJson();
    Serial.println("[main] body: " + body);

    auto res = cell.post(SERVER_HOST, SERVER_PORT, SERVER_PATH, body).wait(10000);
    if (res.has_value()) {
        auto response = res.value();
        Serial.println("[main] POST " + String(response.code));
    } else {
        Serial.println("[main] POST failed: " + res.error().message);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("[main] starting up...");

    delay(2000);
    esp_task_wdt_deinit();

    GPS gps;
    Cellular cell;

    run(gps, cell);

    Serial.println("[main] sleeping for 60s...");
    esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
    vTaskDelete(NULL);
}
