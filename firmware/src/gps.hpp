// gps.hpp

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(TRACKOWS_GPS_SIM7000G)
#include <TinyGsmClient.h>
#else
#include <TinyGPSPlus.h>
#endif
#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
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
public:
    using FixCallback = std::function<void(const GPSFix&)>;
    using ErrorCallback = std::function<void(const Error&)>;

    GPS()
        : serial_(1),
#if defined(TRACKOWS_GPS_SIM7000G)
          modem_(serial_),
#endif
          event_queue_(xQueueCreateStatic(
              1, sizeof(Event), event_queue_storage_, &event_queue_buffer_)),
          latest_queue_(xQueueCreateStatic(
              1, sizeof(GPSFix), latest_queue_storage_, &latest_queue_buffer_)) {}

    ~GPS() {
        auto stopped = stop();
        if (stopped.valid()) stopped.wait();
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

    // Start is synchronous internally, but returns a standard ready future so
    // callers can use the same wait_for/get flow as other asynchronous APIs.
    std::future<Result<bool>> start() {
        std::promise<Result<bool>> promise;
        auto future = promise.get_future();

        std::unique_lock<std::mutex> lock(state_mutex_);
        if (running_) {
            promise.set_value(true);
            return future;
        }

        if (!event_queue_ || !latest_queue_) {
            Error error("GPS queue initialization failed");
            lock.unlock();
            emitError(error);
            promise.set_value(tl::make_unexpected(error));
            return future;
        }

        xQueueReset(event_queue_);
        xQueueReset(latest_queue_);

        auto hardware = startHardware();
        if (!hardware.has_value()) {
            Error error = hardware.error();
            lock.unlock();
            emitError(error);
            promise.set_value(tl::make_unexpected(error));
            return future;
        }
        running_ = true;

        try {
            dispatch_thread_ = makeThread(
                [this] { dispatchLoop(); }, "gps_events", 4096, 1);
            reader_thread_ = makeThread(
                [this] { readerLoop(); }, "gps_reader", 4096, 2);
            promise.set_value(true);
        } catch (const std::exception& exception) {
            running_ = false;
            if (reader_thread_.joinable()) reader_thread_.join();
            if (dispatch_thread_.joinable()) {
                Event stop{EventType::Stop, {}};
                xQueueOverwrite(event_queue_, &stop);
                dispatch_thread_.join();
            }
            stopHardware();

            Error error(exception.what());
            lock.unlock();
            emitError(error);
            promise.set_value(tl::make_unexpected(error));
        }

        return future;
    }

    std::future<Result<bool>> stop() {
        std::packaged_task<Result<bool>()> task([this]() -> Result<bool> {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!running_.exchange(false)) return true;

            // Stop the producer before posting the sentinel so it cannot
            // overwrite the stop event with one final fix.
            if (reader_thread_.joinable()) reader_thread_.join();

            Event stop{EventType::Stop, {}};
            xQueueOverwrite(event_queue_, &stop);
            if (dispatch_thread_.joinable()) dispatch_thread_.join();

            stopHardware();
            xQueueReset(event_queue_);
            xQueueReset(latest_queue_);
            return true;
        });

        auto future = task.get_future();
        try {
            auto thread = makeThread(
                std::move(task), "gps_stop", 3072, 1);
            thread.detach();
        } catch (const std::exception& exception) {
            Error error(exception.what());
            emitError(error);

            std::promise<Result<bool>> error_promise;
            future = error_promise.get_future();
            error_promise.set_value(tl::make_unexpected(error));
        }
        return future;
    }

    std::future<Result<GPSFix>> fix() {
        std::packaged_task<Result<GPSFix>()> task([this]() -> Result<GPSFix> {
            while (running_) {
                GPSFix fix{};
                if (xQueuePeek(
                        latest_queue_, &fix, pdMS_TO_TICKS(100)) == pdTRUE)
                    return fix;
            }
            return tl::make_unexpected(Error("GPS stopped"));
        });

        auto future = task.get_future();
        try {
            auto thread = makeThread(
                std::move(task), "gps_fix", 2048, 1);
            thread.detach();
        } catch (const std::exception& exception) {
            std::promise<Result<GPSFix>> error_promise;
            future = error_promise.get_future();
            error_promise.set_value(
                tl::make_unexpected(Error(exception.what())));
        }
        return future;
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

    template <typename Fn>
    static std::thread makeThread(
        Fn&& fn,
        const char* name,
        size_t stack_size,
        size_t priority) {

        esp_pthread_cfg_t previous{};
        const bool had_previous = esp_pthread_get_cfg(&previous) == ESP_OK;
        const esp_pthread_cfg_t restore =
            had_previous ? previous : esp_pthread_get_default_config();

        esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
        cfg.stack_size = stack_size;
        cfg.prio = priority;
        cfg.inherit_cfg = false;
        cfg.thread_name = name;
        cfg.pin_to_core = tskNO_AFFINITY;

        if (esp_pthread_set_cfg(&cfg) != ESP_OK)
            throw std::runtime_error("GPS thread configuration failed");

        try {
            std::thread thread(std::forward<Fn>(fn));
            esp_pthread_set_cfg(&restore);
            return thread;
        } catch (...) {
            esp_pthread_set_cfg(&restore);
            throw;
        }
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

    void publishFix(const GPSFix& fix) {
        const Event event{EventType::Fix, fix};
        xQueueOverwrite(latest_queue_, &fix);
        xQueueOverwrite(event_queue_, &event);
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
            }

            vTaskDelay(pdMS_TO_TICKS(1000));
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
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }
#endif
    }

    void dispatchLoop() {
        for (;;) {
            Event event{};
            if (xQueueReceive(event_queue_, &event, portMAX_DELAY) != pdTRUE)
                continue;
            if (event.type == EventType::Stop) return;

            std::vector<FixCallback> callbacks;
            {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                callbacks = fix_callbacks_;
            }
            for (const auto& callback : callbacks)
                callback(event.fix);
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

    HardwareSerial serial_;
#if defined(TRACKOWS_GPS_SIM7000G)
    TinyGsm modem_;
#else
    TinyGPSPlus gps_;
#endif

    StaticQueue_t event_queue_buffer_{};
    uint8_t event_queue_storage_[sizeof(Event)]{};
    QueueHandle_t event_queue_ = nullptr;

    StaticQueue_t latest_queue_buffer_{};
    uint8_t latest_queue_storage_[sizeof(GPSFix)]{};
    QueueHandle_t latest_queue_ = nullptr;

    std::atomic<bool> running_{false};
    std::thread reader_thread_;
    std::thread dispatch_thread_;
    std::mutex state_mutex_;

    std::mutex callback_mutex_;
    std::vector<FixCallback> fix_callbacks_;
    std::vector<ErrorCallback> error_callbacks_;
};
