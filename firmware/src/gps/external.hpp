#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <optional>

#include "../gps.hpp"

class ExternalGpsBackend final : public GpsBackend {
public:
    Result<bool> start() final {
        serial_.begin(BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
        return true;
    }

    void stop() final {
        serial_.end();
    }

    std::optional<GPSFix> poll() final {
        bool updated = false;
        while (serial_.available()) {
            gps_.encode(serial_.read());
            updated = updated || gps_.location.isUpdated();
        }

        if (!updated || !gps_.location.isValid())
            return std::nullopt;

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
    }

    uint32_t pollIntervalMs() const final { return 20; }

private:
    static constexpr int RX_PIN = 22;
    static constexpr int TX_PIN = 21;
    static constexpr int BAUD = 9600;

    HardwareSerial serial_{1};
    TinyGPSPlus gps_;
};
