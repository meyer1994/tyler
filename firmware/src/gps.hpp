// gps.hpp

#pragma once

#include <TinyGPSPlus.h>
#include "./future.hpp"

#define GPS_RX_PIN  22
#define GPS_TX_PIN  21
#define GPS_BAUD    9600

struct GPSFix {
    double   lat;
    double   lng;
    double   altitude;
    uint32_t satellites;
};

class GPS {
public:
    GPS() : serial_(1) {}

    SharedFuture<bool> open() {
        return SharedFuture<bool>([this]() -> SharedFuture<bool>::Result {
            serial_.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
            while (!gps_.location.isValid()) {
                while (serial_.available()) gps_.encode(serial_.read());
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            return true;
        }, "gps_open", 4096);
    }

    SharedFuture<GPSFix> fix() {
        return SharedFuture<GPSFix>([this]() -> SharedFuture<GPSFix>::Result {
            while (gps_.location.age() > 1000) {
                while (serial_.available()) gps_.encode(serial_.read());
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            return GPSFix{
                gps_.location.lat(),
                gps_.location.lng(),
                gps_.altitude.meters(),
                gps_.satellites.value()
            };
        }, "gps_fix", 4096);
    }

private:
    HardwareSerial serial_;
    TinyGPSPlus    gps_;
};
