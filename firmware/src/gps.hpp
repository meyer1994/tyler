// gps.hpp

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(TRACKOWS_GPS_SIM7000G)
#include <TinyGsmClient.h>
#else
#include <TinyGPSPlus.h>
#endif
#include <FreeRTOS/Queue.hpp>
#include <FreeRTOS/Task.hpp>
#include <atomic>
#include <functional>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>
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

class GPSState {
public:
    enum Value : uint8_t {
        Stopped,
        Starting,
        Running,
        Stopping,
        Failed,
    };

    constexpr GPSState(Value value = Stopped) : value_(value) {}
    constexpr operator Value() const { return value_; }

    const char* getName() const {
        switch (value_) {
            case Stopped:  return "stopped";
            case Starting: return "starting";
            case Running:  return "running";
            case Stopping: return "stopping";
            case Failed:   return "failed";
        }
        return "unknown";
    }

    friend constexpr bool operator==(GPSState lhs, GPSState rhs) {
        return lhs.value_ == rhs.value_;
    }

    friend constexpr bool operator!=(GPSState lhs, GPSState rhs) {
        return !(lhs == rhs);
    }

private:
    Value value_;
};

class GPSFixState {
public:
    enum Value : uint8_t {
        Unknown,
        Searching,
        Fixed,
        Lost,
    };

    constexpr GPSFixState(Value value = Unknown) : value_(value) {}
    constexpr operator Value() const { return value_; }

    const char* getName() const {
        switch (value_) {
            case Unknown:   return "unknown";
            case Searching: return "searching";
            case Fixed:     return "fixed";
            case Lost:      return "lost";
        }
        return "unknown";
    }

    friend constexpr bool operator==(GPSFixState lhs, GPSFixState rhs) {
        return lhs.value_ == rhs.value_;
    }

    friend constexpr bool operator!=(GPSFixState lhs, GPSFixState rhs) {
        return !(lhs == rhs);
    }

private:
    Value value_;
};

static_assert(std::is_trivially_copyable<GPSState>::value,
              "GPSState must remain atomic-safe");
static_assert(std::is_trivially_copyable<GPSFixState>::value,
              "GPSFixState must remain atomic-safe");

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

static_assert(std::is_trivially_copyable<GPSFix>::value,
              "GPSFix must be safe to copy through a FreeRTOS queue");

class GPS {
private:
    // ESP-IDF defines StackType_t as one byte, so these template arguments are
    // stack sizes in bytes on ESP32.
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

    class DispatchTask final : public FreeRTOS::StaticTask<4096> {
    public:
        explicit DispatchTask(GPS& owner)
            : FreeRTOS::StaticTask<4096>(tskIDLE_PRIORITY, "gps_events"),
              owner_(owner) {}

        void taskFunction() final {
            for (;;) {
                notifyTake(portMAX_DELAY);
                setPriority(1);
                owner_.dispatchLoop();
                setPriority(tskIDLE_PRIORITY);
                owner_.dispatch_done_.overwrite(true);
            }
        }

    private:
        GPS& owner_;
    };

public:
    using FixCallback = std::function<void(const GPSFix&)>;
    using ErrorCallback = std::function<void(const Error&)>;
    using StateCallback =
        std::function<void(GPSState previous, GPSState current)>;
    using FixStateChangeCallback =
        std::function<void(GPSFixState previous, GPSFixState current)>;

    GPS()
        : serial_(1),
#if defined(TRACKOWS_GPS_SIM7000G)
          modem_(serial_),
#endif
          reader_task_(*this),
          dispatch_task_(*this) {}

    ~GPS() {
        (void)stop();
    }

    GPS(const GPS&) = delete;
    GPS& operator=(const GPS&) = delete;

    GPS& onFix(FixCallback callback) {
        if (callback) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            fix_callbacks_.push_back(std::move(callback));
        }
        return *this;
    }

    GPS& onError(ErrorCallback callback) {
        if (callback) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            error_callbacks_.push_back(std::move(callback));
        }
        return *this;
    }

    GPS& onState(StateCallback callback) {
        if (callback) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            state_callbacks_.push_back(std::move(callback));
        }
        return *this;
    }

    GPS& onFixStateChange(FixStateChangeCallback callback) {
        if (callback) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            fix_state_callbacks_.push_back(std::move(callback));
        }
        return *this;
    }

    Result<bool> start() {
        std::unique_lock<std::mutex> lock(state_mutex_);
        if (running_) return true;
        if (state_ == GPSState::Starting || state_ == GPSState::Stopping)
            return tl::make_unexpected(
                Error("GPS state transition already in progress"));

        GPSState previous_state = state_.exchange(GPSState::Starting);
        lock.unlock();
        emitStateChange(previous_state, GPSState::Starting);
        lock.lock();

        if (!event_queue_.isValid() || !latest_queue_.isValid() ||
            !reader_done_.isValid() || !dispatch_done_.isValid()) {
            Error error("GPS queue initialization failed");
            previous_state = state_.exchange(GPSState::Failed);
            lock.unlock();
            emitError(error);
            emitStateChange(previous_state, GPSState::Failed);
            return tl::make_unexpected(error);
        }

        event_queue_.reset();
        latest_queue_.reset();
        reader_done_.reset();
        dispatch_done_.reset();

        auto hardware = startHardware();
        if (!hardware.has_value()) {
            Error error = hardware.error();
            previous_state = state_.exchange(GPSState::Failed);
            lock.unlock();
            emitError(error);
            emitStateChange(previous_state, GPSState::Failed);
            return tl::make_unexpected(error);
        }

        running_ = true;
        const GPSFixState previous_fix_state =
            fix_state_.exchange(GPSFixState::Searching);
        dispatch_task_.notifyGive();
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

        const Event stop_event{EventType::Stop, {}};
        event_queue_.overwrite(stop_event);
        if (!dispatch_done_.receive(pdMS_TO_TICKS(2000))) {
            emitError(Error("GPS dispatcher did not stop"));
            clean = false;
        }

        lock.lock();
        stopHardware();
        event_queue_.reset();
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
    enum class EventType : uint8_t {
        Fix,
        Stop,
    };

    struct Event {
        EventType type;
        GPSFix fix;
    };

    static_assert(std::is_trivially_copyable<Event>::value,
                  "GPS event must be safe to copy through a FreeRTOS queue");

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

        const Event event{EventType::Fix, fix};
        latest_queue_.overwrite(fix);
        event_queue_.overwrite(event);
    }

    void checkFixTimeout() {
        if (fix_state_ != GPSFixState::Fixed) return;
        if (millis() - last_fix_ms_ < FIX_LOSS_TIMEOUT_MS) return;
        setFixState(GPSFixState::Lost);
    }

    void readerLoop() {
#if defined(TRACKOWS_GPS_SIM7000G)
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

    void dispatchLoop() {
        for (;;) {
            auto event = event_queue_.receive(portMAX_DELAY);
            if (!event) continue;
            if (event->type == EventType::Stop) return;

            std::vector<FixCallback> callbacks;
            {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                callbacks = fix_callbacks_;
            }
            for (const auto& callback : callbacks)
                callback(event->fix);
        }
    }

    void emitError(const Error& error) {
        std::vector<ErrorCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            callbacks = error_callbacks_;
        }
        for (const auto& callback : callbacks)
            callback(error);
    }

    void emitStateChange(GPSState previous, GPSState current) {
        if (previous == current) return;

        std::vector<StateCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            callbacks = state_callbacks_;
        }
        for (const auto& callback : callbacks)
            callback(previous, current);
    }

    void emitFixStateChange(
        GPSFixState previous, GPSFixState current) {
        if (previous == current) return;

        std::vector<FixStateChangeCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            callbacks = fix_state_callbacks_;
        }
        for (const auto& callback : callbacks)
            callback(previous, current);
    }

    void setFixState(GPSFixState current) {
        const GPSFixState previous = fix_state_.exchange(current);
        emitFixStateChange(previous, current);
    }

    HardwareSerial serial_;
#if defined(TRACKOWS_GPS_SIM7000G)
    TinyGsm modem_;
#else
    TinyGPSPlus gps_;
#endif

    FreeRTOS::StaticQueue<Event, 1> event_queue_;
    FreeRTOS::StaticQueue<GPSFix, 1> latest_queue_;
    FreeRTOS::StaticQueue<bool, 1> reader_done_;
    FreeRTOS::StaticQueue<bool, 1> dispatch_done_;

    ReaderTask reader_task_;
    DispatchTask dispatch_task_;

    static constexpr uint32_t FIX_LOSS_TIMEOUT_MS = 5000;

    std::atomic<bool> running_{false};
    std::atomic<GPSState> state_{GPSState::Stopped};
    std::atomic<GPSFixState> fix_state_{GPSFixState::Unknown};
    std::atomic<uint32_t> last_fix_ms_{0};
    std::mutex state_mutex_;

    std::mutex callback_mutex_;
    std::vector<FixCallback> fix_callbacks_;
    std::vector<ErrorCallback> error_callbacks_;
    std::vector<StateCallback> state_callbacks_;
    std::vector<FixStateChangeCallback> fix_state_callbacks_;
};
