#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <optional>
#include <memory>
#include <atomic>

template <typename T>
class Promise;

template <typename T>
class Future {
    friend class Promise<T>;

public:
    struct Result {
        std::optional<T>      value;
        std::optional<String> error;
        bool ok() const { return value.has_value(); }
    };

    bool is_resolved() const { return _state->done.load(); }

    Result get(uint32_t timeout_ms = portMAX_DELAY) {
        if (_state->done.load()) return _state->result;

        TickType_t ticks = (timeout_ms == portMAX_DELAY)
                           ? portMAX_DELAY
                           : pdMS_TO_TICKS(timeout_ms);

        EventBits_t bits = xEventGroupWaitBits(
            _state->group,
            DONE_BIT,
            pdFALSE,  // don't clear — leaves bit set for all waiters
            pdTRUE,
            ticks
        );

        if (bits & DONE_BIT) {
            // full hardware barrier — ensures result write from producer
            // is visible on this core before we read it
            std::atomic_thread_fence(std::memory_order_seq_cst);
            return _state->result;
        }
        return { std::nullopt, String("timeout") };
    }

private:
    static constexpr EventBits_t DONE_BIT = BIT0;

    struct State {
        EventGroupHandle_t group  = xEventGroupCreate();
        Result             result = {};
        std::atomic<bool>  done   = { false };
        ~State() { if (group) vEventGroupDelete(group); }
    };

    explicit Future(std::shared_ptr<State> s) : _state(std::move(s)) {}
    std::shared_ptr<State> _state;
};

template <typename T>
class Promise {
public:
    Promise() : _state(std::make_shared<typename Future<T>::State>()) {}

    Future<T> get_future() { return Future<T>(_state); }

    void resolve(T value) {
        bool expected = false;
        if (!_state->done.compare_exchange_strong(expected, true)) return;
        _state->result = { std::move(value), std::nullopt };
        // full hardware barrier — flushes result write before setting event bit
        std::atomic_thread_fence(std::memory_order_seq_cst);
        xEventGroupSetBits(_state->group, DONE_BIT);
    }

    void reject(String reason) {
        bool expected = false;
        if (!_state->done.compare_exchange_strong(expected, true)) return;
        _state->result = { std::nullopt, std::move(reason) };
        std::atomic_thread_fence(std::memory_order_seq_cst);
        xEventGroupSetBits(_state->group, DONE_BIT);
    }

private:
    static constexpr EventBits_t DONE_BIT = BIT0;
    std::shared_ptr<typename Future<T>::State> _state;
};