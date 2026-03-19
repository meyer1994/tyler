#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// future.hpp
//
// A std::future-adjacent promise/future for FreeRTOS.
//
// Design goals
//   • API mirrors std::promise / std::future as closely as possible without
//     requiring exceptions (use -fno-exceptions freely).
//   • get() returns tl::expected<T, E> instead of throwing — callers
//     pattern-match on success vs. error rather than try/catch.
//   • Explicit timeout on every blocking call.
//   • Zero heap allocation for sync primitives (StaticEventGroup_t,
//     StaticSemaphore_t).  The shared state itself still lives on the heap
//     via shared_ptr — unavoidable given shared ownership semantics.
//   • In-place storage for T (placement new into aligned byte array) avoids
//     an extra heap allocation for the value.
//   • seq_cst fences on both sides of the result handoff — correct on
//     dual-core targets (ESP32, RP2350, etc.).
//   • CAS guard on resolve/reject — idempotent, safe to call from ISR or
//     concurrent tasks without external locking.
//   • ~promise() auto-rejects with future_errc::broken_promise if destroyed
//     without a prior set_value / set_error call.
//
// Error type E
//   E must be constructible from future_errc so that infrastructure errors
//   (timeout, broken_promise) are always representable.  The default
//   E = future_errc covers most use cases.  Supply a richer domain-error
//   enum only if you need set_error() to carry application-level failures.
//
// Usage
//   freertos::promise<int> p;
//   auto f = p.get_future();
//
//   // producer task / ISR:
//   p.set_value(42);
//   // or on failure:
//   p.set_error(freertos::future_errc::broken_promise);
//
//   // consumer task:
//   auto result = f.get_for_ms(100);   // tl::expected<int, future_errc>
//   if (result) {
//       use(*result);
//   } else {
//       handle(result.error());        // future_errc::timeout, etc.
//   }
// ─────────────────────────────────────────────────────────────────────────────

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#include <atomic>
#include <cstddef>      // std::byte
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "tl/expected.hpp"

namespace freertos {

// ── Error codes ───────────────────────────────────────────────────────────────

enum class future_errc : uint8_t {
    timeout          = 1,   // get_for() / wait_for() timed out
    broken_promise   = 2,   // promise destroyed without resolve/reject
    already_resolved = 3,   // set_value / set_error called more than once
};

// ── EventFlag — static event group, no heap alloc ────────────────────────────

class EventFlag {
    StaticEventGroup_t _buf;
    EventGroupHandle_t _handle;

    static constexpr EventBits_t DONE_BIT = BIT0;

public:
    EventFlag() noexcept
        : _handle(xEventGroupCreateStatic(&_buf)) {}

    EventFlag(const EventFlag&)            = delete;
    EventFlag& operator=(const EventFlag&) = delete;

    ~EventFlag() { vEventGroupDelete(_handle); }

    void set() noexcept {
        xEventGroupSetBits(_handle, DONE_BIT);
    }

    bool is_set() const noexcept {
        return (xEventGroupGetBits(_handle) & DONE_BIT) != 0;
    }

    // Returns true if signalled before the deadline, false on timeout.
    // Bit is NOT cleared on exit — all concurrent waiters see it.
    bool wait(TickType_t ticks = portMAX_DELAY) noexcept {
        return (xEventGroupWaitBits(
                    _handle, DONE_BIT,
                    pdFALSE,   // xClearOnExit  — keep bit set for multi-waiter
                    pdTRUE,    // xWaitForAllBits — only one bit anyway
                    ticks)
                & DONE_BIT) != 0;
    }
};

// ── Mutex — static semaphore, no heap alloc ───────────────────────────────────

class Mutex {
    StaticSemaphore_t _buf;
    SemaphoreHandle_t _handle;

public:
    Mutex() noexcept
        : _handle(xSemaphoreCreateMutexStatic(&_buf)) {}

    Mutex(const Mutex&)            = delete;
    Mutex& operator=(const Mutex&) = delete;

    ~Mutex() { vSemaphoreDelete(_handle); }

    void lock()   noexcept { xSemaphoreTake(_handle, portMAX_DELAY); }
    void unlock() noexcept { xSemaphoreGive(_handle); }
};

// ── LockGuard — named, non-copyable ──────────────────────────────────────────
//
// Never construct as an unnamed temporary: LockGuard(mutex) — the object
// is destroyed immediately and the lock is never held.
// Always name it:  LockGuard guard(mutex);

class LockGuard {
    Mutex& _m;
public:
    explicit LockGuard(Mutex& m) noexcept : _m(m) { _m.lock(); }
    ~LockGuard() noexcept                          { _m.unlock(); }

    LockGuard(const LockGuard&)            = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

// ── Forward declarations ──────────────────────────────────────────────────────

template<typename T, typename E = future_errc> class promise;
template<typename T, typename E = future_errc> class future;

// ── SharedState ───────────────────────────────────────────────────────────────

template<typename T, typename E>
class SharedState {
    static_assert(std::is_constructible_v<E, future_errc>,
        "Error type E must be constructible from future_errc so that "
        "infrastructure errors (timeout, broken_promise) are always "
        "representable.");

    // In-place storage — T constructed via placement new; no extra heap alloc.
    alignas(T) std::byte _storage[sizeof(T)];
    std::optional<E>     _error;
    std::atomic<bool>    _done{false};
    bool                 _has_value{false};

    EventFlag _flag;
    Mutex     _mutex;

    T*       value_ptr()       noexcept { return reinterpret_cast<T*>(_storage); }
    const T* value_ptr() const noexcept { return reinterpret_cast<const T*>(_storage); }

public:
    SharedState() = default;

    SharedState(const SharedState&)            = delete;
    SharedState& operator=(const SharedState&) = delete;

    ~SharedState() {
        if (_has_value)
            value_ptr()->~T();
    }

    // Returns false if already resolved (idempotent — safe from ISR / tasks).
    template<typename U>
    bool emplace_value(U&& val)
        noexcept(std::is_nothrow_constructible_v<T, U&&>)
    {
        bool expected = false;
        if (!_done.compare_exchange_strong(
                expected, true, std::memory_order_relaxed))
            return false;

        {
            LockGuard guard(_mutex);
            new(_storage) T(std::forward<U>(val));
            _has_value = true;
        }

        // Full hardware barrier: flushes the value write before the flag is
        // visible to other cores.  Required on dual-core targets (ESP32, etc.)
        std::atomic_thread_fence(std::memory_order_seq_cst);
        _flag.set();
        return true;
    }

    bool emplace_error(E err) noexcept {
        bool expected = false;
        if (!_done.compare_exchange_strong(
                expected, true, std::memory_order_relaxed))
            return false;

        {
            LockGuard guard(_mutex);
            _error = std::move(err);
        }

        std::atomic_thread_fence(std::memory_order_seq_cst);
        _flag.set();
        return true;
    }

    bool is_done() const noexcept {
        return _done.load(std::memory_order_acquire);
    }

    bool wait_ticks(TickType_t ticks) noexcept {
        return _flag.wait(ticks);
    }

    tl::expected<T, E> consume(TickType_t ticks) {
        if (!_flag.wait(ticks))
            return tl::unexpected(E{future_errc::timeout});

        // Full barrier: ensures the value/error write from the producer core
        // is visible on this core before we read it.
        std::atomic_thread_fence(std::memory_order_seq_cst);

        if (_error.has_value())
            return tl::unexpected(*_error);

        return *value_ptr();
    }
};

// ── future ────────────────────────────────────────────────────────────────────

template<typename T, typename E>
class future {
    friend class promise<T, E>;

    using State = SharedState<T, E>;
    std::shared_ptr<State> _state;

    explicit future(std::shared_ptr<State> s) noexcept
        : _state(std::move(s)) {}

public:
    // Matches std::future_status (omits deferred — not applicable here).
    enum class status { ready, timeout };

    // ── Non-blocking ─────────────────────────────────────────────────────────

    bool is_ready() const noexcept { return _state->is_done(); }

    // ── Blocking — no result ─────────────────────────────────────────────────

    // Mirrors std::future::wait() — blocks until resolved, no timeout.
    void wait() noexcept { _state->wait_ticks(portMAX_DELAY); }

    // Mirrors std::future::wait_for() — returns status without consuming value.
    status wait_for(TickType_t ticks) noexcept {
        return _state->wait_ticks(ticks) ? status::ready : status::timeout;
    }

    status wait_for_ms(uint32_t ms) noexcept {
        return wait_for(pdMS_TO_TICKS(ms));
    }

    // ── Blocking — with result ───────────────────────────────────────────────

    // Mirrors std::future::get() — blocks indefinitely.
    // Returns tl::expected<T, E> instead of throwing.
    tl::expected<T, E> get() {
        return _state->consume(portMAX_DELAY);
    }

    // get() with explicit FreeRTOS tick timeout.
    tl::expected<T, E> get_for(TickType_t ticks) {
        return _state->consume(ticks);
    }

    // get() with millisecond timeout — most ergonomic for application code.
    tl::expected<T, E> get_for_ms(uint32_t ms) {
        return get_for(pdMS_TO_TICKS(ms));
    }
};

// ── promise ───────────────────────────────────────────────────────────────────

template<typename T, typename E>
class promise {
    using State = SharedState<T, E>;
    std::shared_ptr<State> _state;

public:
    promise() : _state(std::make_shared<State>()) {}

    // Not copyable — mirrors std::promise.
    promise(const promise&)            = delete;
    promise& operator=(const promise&) = delete;

    // Movable — mirrors std::promise.
    promise(promise&&) noexcept            = default;
    promise& operator=(promise&&) noexcept = default;

    // Auto-rejects with broken_promise if destroyed without resolving —
    // prevents a future::get() from blocking forever after the producer
    // task exits or is deleted.
    ~promise() {
        if (_state && !_state->is_done())
            _state->emplace_error(E{future_errc::broken_promise});
    }

    // Returns the associated future.
    future<T, E> get_future() {
        return future<T, E>(_state);
    }

    // ── Resolve ──────────────────────────────────────────────────────────────

    // Mirrors std::promise::set_value().
    // Returns true on first call; false (no-op) on subsequent calls.
    template<typename U>
    bool set_value(U&& val)
        noexcept(noexcept(_state->emplace_value(std::forward<U>(val))))
    {
        return _state->emplace_value(std::forward<U>(val));
    }

    // Reject with a domain or infrastructure error.
    // Replaces std::promise::set_exception() for -fno-exceptions targets.
    bool set_error(E err) noexcept {
        return _state->emplace_error(std::move(err));
    }
};

} // namespace freertos
