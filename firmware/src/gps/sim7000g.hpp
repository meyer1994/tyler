#pragma once

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <optional>

#include "../gps.hpp"

class Sim7000GpsBackend final : public GpsBackend {
public:
    Result<bool> start() final {
        pinMode(MODEM_DTR_PIN, OUTPUT);
        digitalWrite(MODEM_DTR_PIN, LOW);

        pinMode(MODEM_PWRKEY_PIN, OUTPUT);
        digitalWrite(MODEM_PWRKEY_PIN, LOW);
        delay(100);
        digitalWrite(MODEM_PWRKEY_PIN, HIGH);
        delay(1000);
        digitalWrite(MODEM_PWRKEY_PIN, LOW);

        serial_.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
        delay(3000);

        bool responding = false;
        for (int attempt = 0; attempt < 30 && !responding; ++attempt)
            responding = modem_.testAT(1000);
        if (!responding)
            return tl::make_unexpected(Error("SIM7000G is not responding"));

        if (!modem_.enableGPS(GNSS_ENABLE_GPIO, 1))
            return tl::make_unexpected(Error("SIM7000G GNSS enable failed"));

        return true;
    }

    void stop() final {
        modem_.disableGPS(GNSS_ENABLE_GPIO, 0);
        serial_.end();
    }

    std::optional<GPSFix> poll() final {
        uint8_t status = 0;
        float lat = 0;
        float lon = 0;
        float speed_knots = 0;
        float altitude = 0;
        float hdop = 0;
        int visible_satellites = 0;
        int used_satellites = 0;
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;

        if (!modem_.getGPS(
                &status, &lat, &lon, &speed_knots, &altitude,
                &visible_satellites, &used_satellites, &hdop,
                &year, &month, &day, &hour, &minute, &second))
            return std::nullopt;

        return GPSFix{
            lat,
            lon,
            altitude,
            speed_knots * 1.852,
            0.0,
            hdop,
            static_cast<uint32_t>(used_satellites),
            static_cast<uint16_t>(year),
            static_cast<uint8_t>(month),
            static_cast<uint8_t>(day),
            static_cast<uint8_t>(hour),
            static_cast<uint8_t>(minute),
            static_cast<uint8_t>(second),
        };
    }

    uint32_t pollIntervalMs() const final { return 1000; }

private:
    static constexpr int MODEM_RX_PIN = 26;
    static constexpr int MODEM_TX_PIN = 27;
    static constexpr int MODEM_DTR_PIN = 25;
    static constexpr int MODEM_PWRKEY_PIN = 4;
    static constexpr int MODEM_BAUD = 115200;
    static constexpr int GNSS_ENABLE_GPIO = 48;

    HardwareSerial serial_{1};
    TinyGsm modem_{serial_};
};
