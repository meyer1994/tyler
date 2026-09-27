#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstring>

#include "./emitter.hpp"
#include "./result.hpp"

class GPSState {
public:
    enum Value : uint8_t { Stopped, Starting, Running, Stopping, Failed };

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
    enum Value : uint8_t { Unknown, Searching, Fixed, Lost };

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
        JsonDocument doc;
        toJson(doc);

        String payload;
        serializeJson(doc, payload);
        return payload;
    }

    void toJson(JsonDocument& doc) const {
        char timestamp[21] = {0};
        snprintf(
            timestamp,
            sizeof(timestamp),
            "%04d-%02d-%02dT%02d:%02d:%02dZ",
            year, month, day, hour, minute, second);

        doc["lat"] = lat;
        doc["lon"] = lng;
        doc["alt"] = altitude;
        doc["speed"] = speed;
        doc["heading"] = heading;
        doc["hdop"] = hdop;
        doc["sat"] = satellites;
        doc["ts"] = timestamp;
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
    bool ok;
    bool intersects;
};

namespace GpsEvent {
inline constexpr char Fix[] = "gps.fix";
inline constexpr char Error[] = "gps.error";
inline constexpr char StateChanged[] = "gps.state_changed";
inline constexpr char FixStateChanged[] = "gps.fix_state_changed";
}  // namespace GpsEvent

namespace HttpEvent {
inline constexpr char PostLocation[] = "http.post_location";
inline constexpr char Response[] = "http.response";
inline constexpr char Error[] = "http.error";
}  // namespace HttpEvent
