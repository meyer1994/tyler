#pragma once

#include <esp_err.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

struct NoPayload {};

template <auto E>
struct EventPayload;

template <auto E>
using EventPayloadT = typename EventPayload<E>::type;

template <typename Payload>
struct EventHandler {
    using type = void (*)(const Payload&, void*);
};

template <>
struct EventHandler<NoPayload> {
    using type = void (*)(void*);
};

template <
    typename EventEnum,
    size_t MaxHandlers = 8,
    size_t HandlerStorageSize = 16>
class EventEmitter {
    static_assert(
        std::is_enum<EventEnum>::value,
        "EventEmitter requires an enum type");

public:
    using AfterHandler = void (*)(void* context);

    explicit EventEmitter(esp_event_base_t base)
        : base_(base) {}

    ~EventEmitter() {
        if (loop_) (void)end();
    }

    EventEmitter(const EventEmitter&) = delete;
    EventEmitter& operator=(const EventEmitter&) = delete;

    esp_err_t begin(
        const char* task_name,
        size_t queue_size = 16,
        uint32_t stack_size = 4096,
        UBaseType_t priority = 5,
        BaseType_t core = tskNO_AFFINITY) {
        if (loop_) return ESP_ERR_INVALID_STATE;

        esp_event_loop_args_t args{};
        args.queue_size = static_cast<int32_t>(queue_size);
        args.task_name = task_name;
        args.task_priority = priority;
        args.task_stack_size = stack_size;
        args.task_core_id = core;

        return esp_event_loop_create(&args, &loop_);
    }

    esp_err_t end() {
        if (!loop_) return ESP_ERR_INVALID_STATE;

        const esp_err_t result = esp_event_loop_delete(loop_);
        loop_ = nullptr;
        return result;
    }

    bool started() const {
        return loop_ != nullptr;
    }

    template <EventEnum E>
    using Handler = typename EventHandler<EventPayloadT<E>>::type;

    esp_err_t after(
        uint32_t milliseconds,
        AfterHandler handler,
        void* context = nullptr) {
        if (!loop_) return ESP_ERR_INVALID_STATE;
        if (!handler) return ESP_ERR_INVALID_ARG;

        auto* call = new (std::nothrow) AfterCall{
            nullptr,
            [handler, context] {
                handler(context);
            }};
        if (!call) return ESP_ERR_NO_MEM;

        esp_timer_create_args_t args{};
        args.callback = &timer_callback;
        args.arg = call;
        args.dispatch_method = ESP_TIMER_TASK;
        args.name = "emitter_after";

        esp_err_t result = esp_timer_create(&args, &call->timer);
        if (result == ESP_OK) {
            result = esp_timer_start_once(
                call->timer,
                static_cast<uint64_t>(milliseconds) * 1000ULL);
        }
        if (result != ESP_OK) {
            if (call->timer) (void)esp_timer_delete(call->timer);
            delete call;
        }
        return result;
    }

    template <EventEnum E>
    esp_err_t on(Handler<E> handler, void* context = nullptr) {
        using Payload = EventPayloadT<E>;

        static_assert(
            std::is_same<Payload, NoPayload>::value ||
                std::is_trivially_copyable<Payload>::value,
            "Event payload must be trivially copyable");
        static_assert(
            sizeof(Handler<E>) <= HandlerStorageSize,
            "HandlerStorageSize is too small");

        if (!loop_) return ESP_ERR_INVALID_STATE;
        if (!handler) return ESP_ERR_INVALID_ARG;
        if (handler_count_ >= MaxHandlers) return ESP_ERR_NO_MEM;

        Slot& slot = slots_[handler_count_];
        slot.context = context;
        slot.invoke = &invoke<E>;
        std::memcpy(slot.handler.data(), &handler, sizeof(handler));

        const esp_err_t result = esp_event_handler_register_with(
            loop_,
            base_,
            static_cast<int32_t>(E),
            &trampoline,
            &slot);
        if (result == ESP_OK) ++handler_count_;
        return result;
    }

    template <
        EventEnum E,
        typename std::enable_if<
            !std::is_same<EventPayloadT<E>, NoPayload>::value,
            int>::type = 0>
    esp_err_t emit(
        const EventPayloadT<E>& payload,
        TickType_t timeout = portMAX_DELAY) {
        using Payload = EventPayloadT<E>;
        static_assert(
            std::is_trivially_copyable<Payload>::value,
            "Event payload must be trivially copyable");

        if (!loop_) return ESP_ERR_INVALID_STATE;
        return esp_event_post_to(
            loop_,
            base_,
            static_cast<int32_t>(E),
            const_cast<Payload*>(&payload),
            sizeof(Payload),
            timeout);
    }

    template <
        EventEnum E,
        typename std::enable_if<
            std::is_same<EventPayloadT<E>, NoPayload>::value,
            int>::type = 0>
    esp_err_t emit(TickType_t timeout = portMAX_DELAY) {
        if (!loop_) return ESP_ERR_INVALID_STATE;
        return esp_event_post_to(
            loop_,
            base_,
            static_cast<int32_t>(E),
            nullptr,
            0,
            timeout);
    }

private:
    struct AfterCall {
        esp_timer_handle_t timer;
        std::function<void()> handler;
    };

    static void timer_callback(void* argument) {
        auto* call = static_cast<AfterCall*>(argument);
        auto handler = std::move(call->handler);

        // One-shot timers are inactive when their callback starts. Delete the
        // timer and closure before invoking user code so it can schedule a new
        // callback without consuming a permanent timer slot.
        (void)esp_timer_delete(call->timer);
        delete call;

        // This runs on the ESP timer task. Callbacks must be short and
        // non-blocking; emit events or queue work for longer operations.
        handler();
    }

    struct Slot;

    using Invoker = void (*)(Slot*, void* event_data);

    struct Slot {
        void* context = nullptr;
        Invoker invoke = nullptr;
        std::array<std::byte, HandlerStorageSize> handler{};
    };

    static void trampoline(
        void* argument,
        esp_event_base_t,
        int32_t,
        void* event_data) {
        auto* slot = static_cast<Slot*>(argument);
        slot->invoke(slot, event_data);
    }

    template <EventEnum E>
    static void invoke(Slot* slot, void* event_data) {
        using Payload = EventPayloadT<E>;
        using TypedHandler = Handler<E>;

        TypedHandler handler;
        std::memcpy(&handler, slot->handler.data(), sizeof(handler));

        if constexpr (std::is_same<Payload, NoPayload>::value) {
            handler(slot->context);
        } else {
            const auto& payload = *static_cast<const Payload*>(event_data);
            handler(payload, slot->context);
        }
    }

    esp_event_base_t base_;
    esp_event_loop_handle_t loop_ = nullptr;
    std::array<Slot, MaxHandlers> slots_{};
    size_t handler_count_ = 0;
};
