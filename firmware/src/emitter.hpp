#pragma once

#include <esp_event.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdint>
#include <memory>
#include <new>
#include <mutex>
#include <type_traits>
#include <vector>

struct NoPayload {
    uint8_t unused = 0;
};

template <auto Event>
struct EventPayload;

template <typename EventEnum>
struct EventTraits;

template <typename EventEnum>
class EventEmitter {
    static_assert(std::is_enum<EventEnum>::value,
                  "EventEmitter requires an enum type");

    template <EventEnum Event>
    using Payload = typename EventPayload<Event>::type;

    struct RegistrationBase {
        int32_t event_id;
        esp_event_handler_instance_t instance = nullptr;

        explicit RegistrationBase(int32_t id) : event_id(id) {}
        virtual ~RegistrationBase() = default;
    };

    struct DelayedLoopState {
        std::mutex mutex;
        esp_event_loop_handle_t loop = nullptr;
        uint32_t generation = 0;
    };

    template <EventEnum Event>
    struct DelayedEvent {
        std::shared_ptr<DelayedLoopState> state;
        Payload<Event> payload;
        esp_timer_handle_t timer = nullptr;
        uint32_t generation;

        static void dispatch(void* argument) {
            auto* delayed = static_cast<DelayedEvent*>(argument);
            {
                std::lock_guard<std::mutex> lock(delayed->state->mutex);
                if (delayed->state->loop &&
                    delayed->state->generation == delayed->generation) {
                    (void)esp_event_post_to(
                        delayed->state->loop,
                        EventTraits<EventEnum>::base(),
                        static_cast<int32_t>(Event),
                        &delayed->payload,
                        sizeof(delayed->payload),
                        0);
                }
            }

            (void)esp_timer_delete(delayed->timer);
            delete delayed;
        }
    };

    template <EventEnum Event>
    struct Registration final : RegistrationBase {
        using Handler = void (*)(const Payload<Event>&, void*);

        Handler handler;
        void* context;

        Registration(Handler callback, void* callback_context)
            : RegistrationBase(static_cast<int32_t>(Event)),
              handler(callback),
              context(callback_context) {}

        static void dispatch(
            void* argument,
            esp_event_base_t,
            int32_t,
            void* event_data) {
            auto* registration = static_cast<Registration*>(argument);
            registration->handler(
                *static_cast<const Payload<Event>*>(event_data),
                registration->context);
        }
    };

public:
    EventEmitter(
        const char* task_name,
        int32_t queue_size = 16,
        uint32_t task_stack_size = 4096,
        UBaseType_t task_priority = 1,
        BaseType_t task_core = tskNO_AFFINITY)
        : task_name_(task_name),
          queue_size_(queue_size),
          task_stack_size_(task_stack_size),
          task_priority_(task_priority),
          task_core_(task_core) {}

    ~EventEmitter() { stop(); }

    EventEmitter(const EventEmitter&) = delete;
    EventEmitter& operator=(const EventEmitter&) = delete;

    esp_err_t start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (loop_) return ESP_OK;

        esp_event_loop_args_t args{};
        args.queue_size = queue_size_;
        args.task_name = task_name_;
        args.task_priority = task_priority_;
        args.task_stack_size = task_stack_size_;
        args.task_core_id = task_core_;
        const esp_err_t result = esp_event_loop_create(&args, &loop_);
        if (result == ESP_OK) {
            std::lock_guard<std::mutex> delayed_lock(delayed_state_->mutex);
            delayed_state_->loop = loop_;
            ++delayed_state_->generation;
        }
        return result;
    }

    void stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!loop_) return;

        {
            std::lock_guard<std::mutex> delayed_lock(delayed_state_->mutex);
            delayed_state_->loop = nullptr;
            ++delayed_state_->generation;
        }

        for (const auto& registration : registrations_) {
            esp_event_handler_instance_unregister_with(
                loop_,
                EventTraits<EventEnum>::base(),
                registration->event_id,
                registration->instance);
        }
        registrations_.clear();
        esp_event_loop_delete(loop_);
        loop_ = nullptr;
    }

    bool started() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return loop_ != nullptr;
    }

    template <EventEnum Event>
    esp_err_t on(
        void (*handler)(const Payload<Event>&, void*),
        void* context = nullptr) {
        static_assert(
            std::is_trivially_copyable<Payload<Event>>::value,
            "event payloads must be trivially copyable");

        if (!handler) return ESP_ERR_INVALID_ARG;

        std::lock_guard<std::mutex> lock(mutex_);
        if (!loop_) return ESP_ERR_INVALID_STATE;

        auto registration =
            std::make_unique<Registration<Event>>(handler, context);
        const esp_err_t result = esp_event_handler_instance_register_with(
            loop_,
            EventTraits<EventEnum>::base(),
            static_cast<int32_t>(Event),
            &Registration<Event>::dispatch,
            registration.get(),
            &registration->instance);
        if (result != ESP_OK) return result;

        registrations_.push_back(std::move(registration));
        return ESP_OK;
    }

    template <EventEnum Event>
    esp_err_t emit(
        const Payload<Event>& payload,
        TickType_t timeout = 0) const {
        static_assert(
            std::is_trivially_copyable<Payload<Event>>::value,
            "event payloads must be trivially copyable");

        std::lock_guard<std::mutex> lock(mutex_);
        if (!loop_) return ESP_ERR_INVALID_STATE;
        return esp_event_post_to(
            loop_,
            EventTraits<EventEnum>::base(),
            static_cast<int32_t>(Event),
            const_cast<Payload<Event>*>(&payload),
            sizeof(payload),
            timeout);
    }

    template <EventEnum Event,
              typename std::enable_if<
                  std::is_same<Payload<Event>, NoPayload>::value,
                  int>::type = 0>
    esp_err_t emit(TickType_t timeout = 0) const {
        return emit<Event>(NoPayload{}, timeout);
    }

    template <EventEnum Event>
    esp_err_t emitAfter(
        const Payload<Event>& payload,
        uint64_t delay_ms) const {
        static_assert(
            std::is_trivially_copyable<Payload<Event>>::value,
            "event payloads must be trivially copyable");

        if (delay_ms == 0) return emit<Event>(payload);
        if (delay_ms > UINT64_MAX / 1000) return ESP_ERR_INVALID_ARG;

        uint32_t generation;
        {
            std::lock_guard<std::mutex> lock(delayed_state_->mutex);
            if (!delayed_state_->loop) return ESP_ERR_INVALID_STATE;
            generation = delayed_state_->generation;
        }

        auto* delayed = new (std::nothrow) DelayedEvent<Event>{
            delayed_state_, payload, nullptr, generation};
        if (!delayed) return ESP_ERR_NO_MEM;

        esp_timer_create_args_t args{};
        args.callback = &DelayedEvent<Event>::dispatch;
        args.arg = delayed;
        args.dispatch_method = ESP_TIMER_TASK;
        args.name = task_name_;

        esp_err_t result = esp_timer_create(&args, &delayed->timer);
        if (result == ESP_OK)
            result = esp_timer_start_once(delayed->timer, delay_ms * 1000);
        if (result != ESP_OK) {
            if (delayed->timer) (void)esp_timer_delete(delayed->timer);
            delete delayed;
        }
        return result;
    }

    template <EventEnum Event,
              typename std::enable_if<
                  std::is_same<Payload<Event>, NoPayload>::value,
                  int>::type = 0>
    esp_err_t emitAfter(uint64_t delay_ms) const {
        return emitAfter<Event>(NoPayload{}, delay_ms);
    }

private:
    const char* task_name_;
    int32_t queue_size_;
    uint32_t task_stack_size_;
    UBaseType_t task_priority_;
    BaseType_t task_core_;

    mutable std::mutex mutex_;
    esp_event_loop_handle_t loop_ = nullptr;
    std::vector<std::unique_ptr<RegistrationBase>> registrations_;
    std::shared_ptr<DelayedLoopState> delayed_state_ =
        std::make_shared<DelayedLoopState>();
};
