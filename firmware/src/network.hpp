#pragma once

#include <Arduino.h>

#include "./result.hpp"

struct NetworkResponse {
    int code;
    String body;
};

class Network {
public:
    virtual ~Network() = default;

    virtual bool connected() const = 0;
    virtual String macAddress() const = 0;  // "AA:BB:CC:DD:EE:FF"
    virtual Result<String> ip() const = 0;
    virtual Result<NetworkResponse> post(
        const String& url,
        const char* content_type,
        const String& body) = 0;
};
