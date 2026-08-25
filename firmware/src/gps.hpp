#pragma once

#include <Arduino.h>
#if defined(TRACKOWS_GPS_SIM7000G)
#include <TinyGsmClient.h>
#else
#include <TinyGPSPlus.h>
#endif
#include <FreeRTOS/Queue.hpp>
#include <FreeRTOS/Task.hpp>
#include <atomic>
#include <mutex>
#include <type_traits>

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
private:
    // ESP-IDF defines StackType_t as one byte, so this is an 8 KiB stack.
    class ReaderTask final : public FreeRTOS::StaticTask<8192> {
    public:
        explicit ReaderTask(GPS& owner)
            : FreeRTOS::StaticTask<8192>(tskIDLE_PRIORITY, "gps_reader"),
              owner_(owner) {}

        void taskFunction() final {
            for (;;) {
                notifyTake(portMAX_DELAY);
                setPriority(2);
                owner_.readerLoop();
                setPriority(tskIDLE_PRIORITY);
                owner_.reader_done_.overwrite(true);
            }
        }

    private:
        GPS& owner_;
    };

public:
    explicit GPS(GpsEmitter& events)
        : events_(events),
          serial_(1),
#if defined(TRACKOWS_GPS_SIM7000G)
          modem_(serial_),
#endif
          reader_task_(*this) {}

    ~GPS() { (void)stop(); }

    GPS(const GPS&) = delete;
    GPS& operator=(const GPS&) = delete;

    Result<bool> start() {
        std::unique_lock<std::mutex> lock(state_mutex_);
        if (running_) return true;
        if (!events_.started())
            return tl::make_unexpected(Error("GPS event loop is not started"));
        if (state_ == GPSState::Starting || state_ == GPSState::Stopping)
            return tl::make_unexpected(
                Error("GPS state transition already in progress"));

        GPSState previous_state = state_.exchange(GPSState::Starting);
        lock.unlock();
        emitStateChange(previous_state, GPSState::Starting);
        lock.lock();

        if (!latest_queue_.isValid() || !reader_done_.isValid()) {
            const Error error("GPS queue initialization failed");
            previous_state = state_.exchange(GPSState::Failed);
            lock.unlock();
            emitError(error);
            emitStateChange(previous_state, GPSState::Failed);
            return tl::make_unexpected(error);
        }

        latest_queue_.reset();
        reader_done_.reset();

        auto hardware = startHardware();
        if (!hardware.has_value()) {
            const Error error = hardware.error();
            previous_state = state_.exchange(GPSState::Failed);
            lock.unlock();
            emitError(error);
            emitStateChange(previous_state, GPSState::Failed);
            return tl::make_unexpected(error);
        }

        running_ = true;
        const GPSFixState previous_fix_state =
            fix_state_.exchange(GPSFixState::Searching);
        reader_task_.notifyGive();
        previous_state = state_.exchange(GPSState::Running);
        lock.unlock();

        emitFixStateChange(previous_fix_state, GPSFixState::Searching);
        emitStateChange(previous_state, GPSState::Running);
        return true;
    }

    Result<bool> stop() {
        std::unique_lock<std::mutex> lock(state_mutex_);
        if (!running_.exchange(false)) return true;

        GPSState previous_state = state_.exchange(GPSState::Stopping);
        lock.unlock();
        emitStateChange(previous_state, GPSState::Stopping);

        bool clean = true;
        if (!reader_done_.receive(pdMS_TO_TICKS(15000))) {
            emitError(Error("GPS reader did not stop"));
            clean = false;
        }

        lock.lock();
        stopHardware();
        latest_queue_.reset();

        const GPSFixState previous_fix_state =
            fix_state_.exchange(GPSFixState::Unknown);
        previous_state = state_.exchange(GPSState::Stopped);
        lock.unlock();

        emitFixStateChange(previous_fix_state, GPSFixState::Unknown);
        emitStateChange(previous_state, GPSState::Stopped);
        if (!clean)
            return tl::make_unexpected(Error("GPS stop incomplete"));
        return true;
    }

    Result<GPSFix> fix(uint32_t timeout_ms = 0) {
        if (!running_)
            return tl::make_unexpected(Error("GPS is not running"));

        auto current = latest_queue_.peek(pdMS_TO_TICKS(timeout_ms));
        if (!current)
            return tl::make_unexpected(Error("GPS fix unavailable"));
        return *current;
    }

private:
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

    void publishFix(const GPSFix& fix) {
        last_fix_ms_ = millis();
        setFixState(GPSFixState::Fixed);
        latest_queue_.overwrite(fix);

        const esp_err_t result = events_.emit<GpsEvent::Fix>(fix);
        if (result != ESP_OK)
            Serial.println(
                "[gps] fix event dropped: " + String(esp_err_to_name(result)));
    }

    void checkFixTimeout() {
        if (fix_state_ != GPSFixState::Fixed) return;
        if (millis() - last_fix_ms_ < FIX_LOSS_TIMEOUT_MS) return;
        setFixState(GPSFixState::Lost);
    }

    void readerLoop() {
#if defined(TRACKOWS_GPS_SIM7000G)
        // TinyGSM's AT interface is blocking. This reader task is the sole
        // owner of HardwareSerial, TinyGsm, and all GNSS modem commands.
        while (running_) {
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

            FreeRTOS::TaskBase::delay(pdMS_TO_TICKS(1000));
        }
#else
        while (running_) {
            bool updated = false;
            while (serial_.available()) {
                gps_.encode(serial_.read());
                updated = updated || gps_.location.isUpdated();
            }

            if (running_ && updated && gps_.location.isValid()) {
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

            FreeRTOS::TaskBase::delay(pdMS_TO_TICKS(20));
        }
#endif
    }

    void emitError(const Error& error) {
        (void)events_.emit<GpsEvent::Error>(eventError(error));
    }

    void emitStateChange(GPSState previous, GPSState current) {
        if (previous == current) return;
        (void)events_.emit<GpsEvent::StateChanged>({previous, current});
    }

    void emitFixStateChange(
        GPSFixState previous,
        GPSFixState current) {
        if (previous == current) return;
        (void)events_.emit<GpsEvent::FixStateChanged>({previous, current});
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

    FreeRTOS::StaticQueue<GPSFix, 1> latest_queue_;
    FreeRTOS::StaticQueue<bool, 1> reader_done_;
    ReaderTask reader_task_;

    static constexpr uint32_t FIX_LOSS_TIMEOUT_MS = 5000;

    std::atomic<bool> running_{false};
    std::atomic<GPSState> state_{GPSState::Stopped};
    std::atomic<GPSFixState> fix_state_{GPSFixState::Unknown};
    std::atomic<uint32_t> last_fix_ms_{0};
    std::mutex state_mutex_;
};
