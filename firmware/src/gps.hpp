// gps.hpp

#pragma once

#include <TinyGPSPlus.h>
#include <ArduinoJson.h>
#include "./future.hpp"

#define GPS_RX_PIN  22
#define GPS_TX_PIN  21
#define GPS_BAUD    9600

struct GPSFix {
    double   lat;
    double   lng;
    double   altitude;
    double   speed;
    double   heading;
    double   hdop;
    uint32_t satellites;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;

    String toJson() const {
        char timestamp[21] = {0};
        snprintf(
            timestamp,
            sizeof(timestamp),
            "%04d-%02d-%02dT%02d:%02d:%02dZ",
            year,
            month,
            day,
            hour,
            minute,
            second
        );

        JsonDocument doc;
        doc["lat"] = lat;
        doc["lon"] = lng;
        doc["alt"] = altitude;
        doc["speed"] = speed;
        doc["heading"] = heading;
        doc["hdop"] = hdop;
        doc["sat"] = satellites;
        doc["ts"] = timestamp;

        String payload;
        serializeJson(doc, payload);

        return payload;
    }
};

class GPS {
public:
    GPS() : serial_(1) {}

    SharedFuture<bool> open() {
        return SharedFuture<bool>([this]() -> SharedFuture<bool>::Result {
            Serial.println("[gps] opening serial...");
            serial_.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

            Serial.println("[gps] waiting for valid location...");
            while (!gps_.location.isValid()) {
                while (serial_.available()) gps_.encode(serial_.read());
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            Serial.println("[gps] location valid, satellites: " + String(gps_.satellites.value()));
            return true;
        }, "gps_open", 4096);
    }

    SharedFuture<GPSFix> fix() {
        return SharedFuture<GPSFix>([this]() -> SharedFuture<GPSFix>::Result {
            Serial.println("[gps] waiting for fresh fix...");
            while (gps_.location.age() > 1000) {
                while (serial_.available()) gps_.encode(serial_.read());
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            Serial.println(
                "[gps] fix acquired: " +
                String(gps_.location.lat(), 6) + ", " +
                String(gps_.location.lng(), 6));

            return GPSFix{
                gps_.location.lat(),
                gps_.location.lng(),
                gps_.altitude.meters(),
                gps_.speed.kmph(),
                gps_.course.deg(),
                gps_.hdop.hdop(),
                gps_.satellites.value(),
                gps_.date.year(),
                gps_.date.month(),
                gps_.date.day(),
                gps_.time.hour(),
                gps_.time.minute(),
                gps_.time.second(),
            };
        }, "gps_fix", 4096);
    }

private:
    HardwareSerial serial_;
    TinyGPSPlus    gps_;
};
