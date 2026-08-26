#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <FreeRTOS/Queue.hpp>
#include <FreeRTOS/Task.hpp>

#include <functional>
#include <vector>

// A small asynchronous event dispatcher backed directly by FreeRTOS.
// Payloads passed to the typed emit overload are copied and freed after all
// listeners have run. The void* overload is for borrowed data (or no data).
class EventEmitter {
private:
    static constexpr size_t EVENT_QUEUE_SIZE = 24;
    class EventTask final : public FreeRTOS::StaticTask<4096> {
    public:
        explicit EventTask(EventEmitter& owner, UBaseType_t priority)
            : FreeRTOS::StaticTask<4096>(priority, "EventEmitter"),
              owner_(owner) {}

        void taskFunction() final {
            owner_._poll();
        }

    private:
        EventEmitter& owner_;
    };

public:
    using Callback = std::function<void(void*)>;

    struct Listener {
        String eventName;
        Callback callback;
    };

    using Deleter = void (*)(void*);

    struct Event {
        char name[32];
        void* data;
        Deleter deleter;
    };

    explicit EventEmitter(UBaseType_t taskPriority = 1)
        : mutex_(xSemaphoreCreateMutex()),
          task_(*this, taskPriority) {}

    ~EventEmitter() {
        while (auto event = queue_.receive(0))
            destroy(*event);

        if (mutex_) vSemaphoreDelete(mutex_);
    }

    EventEmitter(const EventEmitter&) = delete;
    EventEmitter& operator=(const EventEmitter&) = delete;

    void on(const char* eventName, Callback callback) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        listeners_.push_back({String(eventName), std::move(callback)});
        xSemaphoreGive(mutex_);
    }

    bool emit(const char* eventName, void* data = nullptr) {
        Event event{};
        copyName(event.name, eventName);
        event.data = data;
        event.deleter = nullptr;
        return send(event);
    }

    template <typename T>
    bool emit(const char* eventName, const T& data) {
        auto* copy = new T(data);

        Event event{};
        copyName(event.name, eventName);
        event.data = copy;
        event.deleter = &deletePayload<T>;
        if (send(event)) return true;

        delete copy;
        return false;
    }

private:
    static void copyName(char (&destination)[32], const char* source) {
        strncpy(destination, source, sizeof(destination) - 1);
        destination[sizeof(destination) - 1] = '\0';
    }

    bool send(const Event& event) {
        return queue_.sendToBack(event, 0);
    }

    void _poll() {
        for (;;) {
            if (auto event = queue_.receive(portMAX_DELAY))
                dispatch(*event);
        }
    }

    void dispatch(const Event& event) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        for (auto& listener : listeners_) {
            if (listener.eventName == event.name)
                listener.callback(event.data);
        }
        xSemaphoreGive(mutex_);
        destroy(event);
    }

    static void destroy(const Event& event) {
        if (event.deleter) event.deleter(event.data);
    }

    template <typename T>
    static void deletePayload(void* data) {
        delete static_cast<T*>(data);
    }

    std::vector<Listener> listeners_;
    FreeRTOS::StaticQueue<Event, EVENT_QUEUE_SIZE> queue_;
    SemaphoreHandle_t mutex_ = nullptr;
    EventTask task_;
};
