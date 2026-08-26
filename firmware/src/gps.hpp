#pragma once

#include <FreeRTOS/Queue.hpp>
#include <FreeRTOS/Task.hpp>
#include <atomic>
#include <mutex>
#include <optional>

#include "./events.hpp"
#include "./result.hpp"

class GpsBackend {
public:
    virtual ~GpsBackend() = default;

    virtual Result<bool> start() = 0;
    virtual void stop() = 0;
    virtual std::optional<GPSFix> poll() = 0;
    virtual uint32_t pollIntervalMs() const = 0;
};

class GPS {
private:
    class GpsTask final : public FreeRTOS::StaticTask<8192> {
    public:
        explicit GpsTask(GPS& owner)
            : FreeRTOS::StaticTask<8192>(tskIDLE_PRIORITY, "gps"),
              owner_(owner) {}

        void taskFunction() final {
            owner_._poll();
        }

    private:
        GPS& owner_;
    };

public:
    GPS(EventEmitter& events, GpsBackend& backend)
        : events_(events),
          backend_(backend),
          gps_task_(*this) {}

    ~GPS() { (void)stop(); }

    GPS(const GPS&) = delete;
    GPS& operator=(const GPS&) = delete;

    Result<bool> start() {
        if (!poll_queue_.isValid())
            return tl::make_unexpected(Error("GPS queue initialization failed"));

        auto hardware = backend_.start();
        if (!hardware.has_value()) {
            state_ = GPSState::Failed;
            emitError(hardware.error());
            return tl::make_unexpected(hardware.error());
        }

        poll_queue_.reset();
        running_ = true;

        const GPSFixState previous_fix_state = fix_state_.exchange(
            GPSFixState::Searching);
        const GPSState previous_state = state_;
        state_ = GPSState::Running;

        emitFixStateChange(previous_fix_state, GPSFixState::Searching);
        emitStateChange(previous_state, GPSState::Running);
        poll_queue_.sendToBack(true, 0);
        return true;
    }

    Result<bool> stop() {
        running_ = false;
        poll_queue_.sendToBack(true, 0);
        {
            std::lock_guard<std::mutex> hardware_lock(hardware_mutex_);
            backend_.stop();
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
    void _poll() {
        for (;;) {
            if (!poll_queue_.receive(portMAX_DELAY)) continue;

            while (running_) {
                pollOnce();
                if (!running_) break;
                (void)poll_queue_.receive(
                    pdMS_TO_TICKS(backend_.pollIntervalMs()));
            }
        }
    }

    void pollOnce() {
        std::lock_guard<std::mutex> hardware_lock(hardware_mutex_);
        if (!running_) return;

        auto fix = backend_.poll();
        if (fix) publishFix(*fix);
    }

    void publishFix(const GPSFix& fix) {
        setFixState(GPSFixState::Fixed);
        events_.emit(GpsEvent::Fix, fix);
    }

    void emitError(const Error& error) {
        events_.emit(GpsEvent::Error, eventError(error));
    }

    void emitStateChange(GPSState previous, GPSState current) {
        if (previous == current) return;
        events_.emit(GpsEvent::StateChanged, GPSStateChange{previous, current});
    }

    void emitFixStateChange(
        GPSFixState previous,
        GPSFixState current) {
        if (previous == current) return;
        events_.emit(
            GpsEvent::FixStateChanged,
            GPSFixStateChange{previous, current});
    }

    void setFixState(GPSFixState current) {
        const GPSFixState previous = fix_state_.exchange(current);
        emitFixStateChange(previous, current);
    }

    EventEmitter& events_;
    GpsBackend& backend_;
    FreeRTOS::StaticQueue<bool, 1> poll_queue_;
    std::atomic<bool> running_{false};
    GPSState state_{GPSState::Stopped};
    std::atomic<GPSFixState> fix_state_{GPSFixState::Unknown};
    std::mutex hardware_mutex_;
    GpsTask gps_task_;
};
