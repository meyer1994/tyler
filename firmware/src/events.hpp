#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_event.h>
#include <cstring>
#include <type_traits>

#include "./emitter.hpp"
#include "./result.hpp"

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

struct GPSFix {
    double lat;
    double lng;
    double altitude;
    double speed;
    double heading;
    double hdop;
    uint32_t satellites;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

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
            second);

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

struct EventError {
    int code;
    char message[160];
};

inline EventError eventError(const Error& error) {
    EventError payload{};
    payload.code = error.code;
    strlcpy(payload.message, error.message.c_str(), sizeof(payload.message));
    return payload;
}

struct GPSStateChange {
    GPSState previous;
    GPSState current;
};

struct GPSFixStateChange {
    GPSFixState previous;
    GPSFixState current;
};

struct HttpPostLocation {
    GPSFix fix;
};

struct HttpResponse {
    int code;
    uint8_t worker;
};

enum class NetworkState : uint8_t {
    Stopped,
    Connecting,
    Connected,
    Failed,
};

struct NetworkStateChange {
    NetworkState previous;
    NetworkState current;
};

struct NetworkAddress {
    char address[48];
};

enum class GpsEvent : int32_t {
    Fix,
    Error,
    StateChanged,
    FixStateChanged,
};

enum class HttpEvent : int32_t {
    PostLocation,
    Response,
    Error,
};

enum class NetworkEvent : int32_t {
    StateChanged,
    AddressAcquired,
    Error,
};

enum class AppEvent : int32_t {
    Started,
    Error,
};

template <> struct EventPayload<GpsEvent::Fix> { using type = GPSFix; };
template <> struct EventPayload<GpsEvent::Error> { using type = EventError; };
template <> struct EventPayload<GpsEvent::StateChanged> {
    using type = GPSStateChange;
};
template <> struct EventPayload<GpsEvent::FixStateChanged> {
    using type = GPSFixStateChange;
};

template <> struct EventPayload<HttpEvent::PostLocation> {
    using type = HttpPostLocation;
};
template <> struct EventPayload<HttpEvent::Response> {
    using type = HttpResponse;
};
template <> struct EventPayload<HttpEvent::Error> { using type = EventError; };

template <> struct EventPayload<NetworkEvent::StateChanged> {
    using type = NetworkStateChange;
};
template <> struct EventPayload<NetworkEvent::AddressAcquired> {
    using type = NetworkAddress;
};
template <> struct EventPayload<NetworkEvent::Error> {
    using type = EventError;
};

template <> struct EventPayload<AppEvent::Started> { using type = NoPayload; };
template <> struct EventPayload<AppEvent::Error> { using type = EventError; };

ESP_EVENT_DECLARE_BASE(GPS_EVENT_BASE);
ESP_EVENT_DECLARE_BASE(HTTP_EVENT_BASE);
ESP_EVENT_DECLARE_BASE(NETWORK_EVENT_BASE);
ESP_EVENT_DECLARE_BASE(APP_EVENT_BASE);

template <> struct EventTraits<GpsEvent> {
    static esp_event_base_t base() { return GPS_EVENT_BASE; }
};
template <> struct EventTraits<HttpEvent> {
    static esp_event_base_t base() { return HTTP_EVENT_BASE; }
};
template <> struct EventTraits<NetworkEvent> {
    static esp_event_base_t base() { return NETWORK_EVENT_BASE; }
};
template <> struct EventTraits<AppEvent> {
    static esp_event_base_t base() { return APP_EVENT_BASE; }
};

using GpsEmitter = EventEmitter<GpsEvent>;
using HttpEmitter = EventEmitter<HttpEvent>;
using NetworkEmitter = EventEmitter<NetworkEvent>;
using AppEmitter = EventEmitter<AppEvent>;

static_assert(std::is_trivially_copyable<GPSFix>::value,
              "GPSFix must be safe to copy through esp_event");
static_assert(std::is_trivially_copyable<EventError>::value,
              "EventError must be safe to copy through esp_event");
