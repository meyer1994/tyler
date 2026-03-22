// future.hpp

#pragma once

#include <Arduino.h>
#include <memory>
#include <functional>
#include <tl/expected.hpp>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

struct Error {
    String message;
    int    code = 0;

    Error() = default;
    Error(const char* msg, int c = 0) : message(msg), code(c) {}
    Error(String msg, int c = 0) : message(msg), code(c) {}
};

template <typename T, typename E = Error>
class SharedFuture {
public:
    using Result = tl::expected<T, E>;
    using TaskFn = std::function<Result()>;

    SharedFuture() : state_(nullptr) {}

    SharedFuture(TaskFn fn, const char* name = "future", uint32_t stack = 4096) {
        state_ = std::make_shared<State>();
        state_->fn = fn;
        state_->event = xEventGroupCreate();
        state_->done = false;

        auto captured = state_;
        xTaskCreate(run, name, stack,
            new std::shared_ptr<State>(captured),
            1, nullptr);
    }

    Result wait(uint32_t timeout_ms = portMAX_DELAY) {
        if (!state_) return tl::make_unexpected(E("no state"));
        if (!state_->done) {
            auto bits = xEventGroupWaitBits(state_->event, DONE_BIT,
                pdFALSE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
            if (!(bits & DONE_BIT))
                return tl::make_unexpected(E("timeout"));
        }
        return state_->result;
    }

    bool is_done() const { return state_ && state_->done; }

    SharedFuture(const SharedFuture&) = default;
    SharedFuture& operator=(const SharedFuture&) = default;
    SharedFuture(SharedFuture&&) = default;
    SharedFuture& operator=(SharedFuture&&) = default;
    ~SharedFuture() = default;

private:
    static constexpr EventBits_t DONE_BIT = (1 << 0);

    struct State {
        TaskFn              fn;
        Result              result = tl::make_unexpected(E("not started"));
        EventGroupHandle_t  event = nullptr;
        volatile bool       done = false;

        ~State() {
            if (event) vEventGroupDelete(event);
        }
    };

    static void run(void* arg) {
        auto* sp = static_cast<std::shared_ptr<State>*>(arg);
        auto s = std::move(*sp);
        delete sp;

        s->result = s->fn();
        s->done = true;
        xEventGroupSetBits(s->event, DONE_BIT);
        vTaskDelete(nullptr);
    }

    std::shared_ptr<State> state_;
};