#pragma once

#include <Arduino.h>
#if defined(TRACKOWS_GPS_SIM7000G)
#include <TinyGsmClient.h>
#else
#include <TinyGPSPlus.h>
#endif
#include <atomic>
#include <mutex>

#include "./events.hpp"
#include "./result.hpp"

#if defined(TRACKOWS_GPS_SIM7000G)
#define GPS_MODEM_RX_PIN      26
#define GPS_MODEM_TX_PIN      27
#define GPS_MODEM_DTR_PIN     25
#define GPS_MODEM_PWRKEY_PIN   4
#define GPS_MODEM_BAUD    115200
#define GPS_MODEM_ENABLE_GPIO 48
#else
#define GPS_RX_PIN  22
#define GPS_TX_PIN  21
#define GPS_BAUD    9600
#endif

class GPS {
public:
    explicit GPS(GpsEmitter& events)
        : events_(events),
          serial_(1)
#if defined(TRACKOWS_GPS_SIM7000G)
          , modem_(serial_)
#endif
    {}

    ~GPS() { (void)stop(); }

    GPS(const GPS&) = delete;
    GPS& operator=(const GPS&) = delete;

    Result<bool> start() {
        (void)events_.on<GpsEvent::Poll>(&GPS::handlePoll, this);
        auto hardware = startHardware();
        if (!hardware.has_value()) {
            state_ = GPSState::Failed;
            emitError(hardware.error());
            return tl::make_unexpected(hardware.error());
        }

        running_ = true;
        const GPSFixState previous_fix_state = fix_state_.exchange(
            GPSFixState::Searching);
        const GPSState previous_state = state_;
        state_ = GPSState::Running;

        emitFixStateChange(previous_fix_state, GPSFixState::Searching);
        emitStateChange(previous_state, GPSState::Running);
        (void)events_.emit<GpsEvent::Poll>(0);
        return true;
    }

    Result<bool> stop() {
        running_ = false;
        {
            std::lock_guard<std::mutex> hardware_lock(hardware_mutex_);
            stopHardware();
        }
        const GPSFixState previous_fix_state = fix_state_.exchange(
            GPSFixState::Unknown);
        const GPSState previous_state = state_;
        state_ = GPSState::Stopped;

        emitFixStateChange(previous_fix_state, GPSFixState::Unknown);
        emitStateChange(previous_state, GPSState::Stopped);
        return true;
    }

private:
    static void handlePoll(void* context) {
        static_cast<GPS*>(context)->pollOnce();
    }

    static void emitPoll(void* context) {
        auto* gps = static_cast<GPS*>(context);
        if (!gps->running_) return;
        (void)gps->events_.emit<GpsEvent::Poll>(0);
    }

    void schedulePoll() {
        if (!running_) return;
        (void)events_.after(POLL_INTERVAL_MS, &GPS::emitPoll, this);
    }

    Result<bool> startHardware() {
#if defined(TRACKOWS_GPS_SIM7000G)
        pinMode(GPS_MODEM_DTR_PIN, OUTPUT);
        digitalWrite(GPS_MODEM_DTR_PIN, LOW);

        pinMode(GPS_MODEM_PWRKEY_PIN, OUTPUT);
        digitalWrite(GPS_MODEM_PWRKEY_PIN, LOW);
        delay(100);
        digitalWrite(GPS_MODEM_PWRKEY_PIN, HIGH);
        delay(1000);
        digitalWrite(GPS_MODEM_PWRKEY_PIN, LOW);

        serial_.begin(
            GPS_MODEM_BAUD, SERIAL_8N1,
            GPS_MODEM_RX_PIN, GPS_MODEM_TX_PIN);
        delay(3000);

        bool responding = false;
        for (int attempt = 0; attempt < 30 && !responding; ++attempt)
            responding = modem_.testAT(1000);
        if (!responding)
            return tl::make_unexpected(Error("SIM7000G is not responding"));

        if (!modem_.enableGPS(GPS_MODEM_ENABLE_GPIO, 1))
            return tl::make_unexpected(Error("SIM7000G GNSS enable failed"));
#else
        serial_.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#endif
        return true;
    }

    void stopHardware() {
#if defined(TRACKOWS_GPS_SIM7000G)
        modem_.disableGPS(GPS_MODEM_ENABLE_GPIO, 0);
#endif
        serial_.end();
    }

    void pollOnce() {
        {
            std::lock_guard<std::mutex> hardware_lock(hardware_mutex_);
            if (!running_) return;

#if defined(TRACKOWS_GPS_SIM7000G)
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

            if (modem_.getGPS(
                    &status, &lat, &lon, &speed_knots, &altitude,
                    &visible_satellites, &used_satellites, &hdop,
                    &year, &month, &day, &hour, &minute, &second)) {
                publishFix(GPSFix{
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
                });
            } else {
                checkFixTimeout();
            }
#else
            bool updated = false;
            while (serial_.available()) {
                gps_.encode(serial_.read());
                updated = updated || gps_.location.isUpdated();
            }

            if (updated && gps_.location.isValid()) {
                publishFix(GPSFix{
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
                });
            } else {
                checkFixTimeout();
            }
#endif
        }

        if (running_) schedulePoll();
    }

    void publishFix(const GPSFix& fix) {
        last_fix_ms_ = millis();
        setFixState(GPSFixState::Fixed);
        (void)events_.emit<GpsEvent::Fix>(fix, 0);
    }

    void checkFixTimeout() {
        if (fix_state_ != GPSFixState::Fixed) return;
        if (millis() - last_fix_ms_ < FIX_LOSS_TIMEOUT_MS) return;
        setFixState(GPSFixState::Lost);
    }

    void emitError(const Error& error) {
        (void)events_.emit<GpsEvent::Error>(eventError(error), 0);
    }

    void emitStateChange(GPSState previous, GPSState current) {
        if (previous == current) return;
        (void)events_.emit<GpsEvent::StateChanged>({previous, current}, 0);
    }

    void emitFixStateChange(
        GPSFixState previous,
        GPSFixState current) {
        if (previous == current) return;
        (void)events_.emit<GpsEvent::FixStateChanged>({previous, current}, 0);
    }

    void setFixState(GPSFixState current) {
        const GPSFixState previous = fix_state_.exchange(current);
        emitFixStateChange(previous, current);
    }

    GpsEmitter& events_;
    HardwareSerial serial_;
#if defined(TRACKOWS_GPS_SIM7000G)
    TinyGsm modem_;
#else
    TinyGPSPlus gps_;
#endif

#if defined(TRACKOWS_GPS_SIM7000G)
    static constexpr uint32_t POLL_INTERVAL_MS = 1000;
#else
    static constexpr uint32_t POLL_INTERVAL_MS = 20;
#endif
    static constexpr uint32_t FIX_LOSS_TIMEOUT_MS = 5000;

    std::atomic<bool> running_{false};
    GPSState state_{GPSState::Stopped};
    std::atomic<GPSFixState> fix_state_{GPSFixState::Unknown};
    std::atomic<uint32_t> last_fix_ms_{0};
    std::mutex hardware_mutex_;
};
