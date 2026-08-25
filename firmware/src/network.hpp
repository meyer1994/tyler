#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include <ctime>
#include <mutex>

#include "./events.hpp"
#include "./result.hpp"

class NetworkService {
public:
    NetworkService(
        NetworkEmitter& events,
        String ssid,
        String password)
        : events_(events),
          ssid_(std::move(ssid)),
          password_(std::move(password)) {}

    ~NetworkService() { (void)stop(); }

    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;

    Result<bool> start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == NetworkState::Connected) return true;
        if (!events_.started())
            return tl::make_unexpected(
                Error("network event loop is not started"));
        if (ssid_.isEmpty())
            return tl::make_unexpected(Error("WiFi is not configured"));

        setState(NetworkState::Connecting);
        Serial.println("[network] connecting to " + ssid_);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid_.c_str(), password_.c_str());

        const uint32_t begin_ms = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - begin_ms >= connect_timeout_ms_) {
                const Error error("WiFi connection timed out");
                WiFi.disconnect(true);
                setState(NetworkState::Failed);
                emitError(error);
                return tl::make_unexpected(error);
            }
            delay(100);
        }

        configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
        const uint32_t time_begin_ms = millis();
        time_t now = time(nullptr);
        while (now < MINIMUM_VALID_TIME) {
            if (millis() - time_begin_ms >= time_sync_timeout_ms_) {
                const Error error("network time synchronization timed out");
                WiFi.disconnect(true);
                setState(NetworkState::Failed);
                emitError(error);
                return tl::make_unexpected(error);
            }
            delay(100);
            now = time(nullptr);
        }

        setState(NetworkState::Connected);
        NetworkAddress address{};
        strlcpy(
            address.address,
            WiFi.localIP().toString().c_str(),
            sizeof(address.address));
        (void)events_.emit<NetworkEvent::AddressAcquired>(address);
        return true;
    }

    Result<bool> stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == NetworkState::Stopped) return true;
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        setState(NetworkState::Stopped);
        return true;
    }

    Result<String> ip() const {
        if (WiFi.status() != WL_CONNECTED)
            return tl::make_unexpected(Error("not connected"));
        return WiFi.localIP().toString();
    }

private:
    void setState(NetworkState current) {
        const NetworkState previous = state_.exchange(current);
        if (previous == current) return;
        (void)events_.emit<NetworkEvent::StateChanged>({previous, current});
    }

    void emitError(const Error& error) {
        (void)events_.emit<NetworkEvent::Error>(eventError(error));
    }

    NetworkEmitter& events_;
    String ssid_;
    String password_;
    static constexpr time_t MINIMUM_VALID_TIME = 1704067200;  // 2024-01-01

    uint32_t connect_timeout_ms_ = 30000;
    uint32_t time_sync_timeout_ms_ = 15000;
    std::atomic<NetworkState> state_{NetworkState::Stopped};
    mutable std::mutex mutex_;
};
