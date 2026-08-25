// result.hpp

#pragma once

#include <Arduino.h>
#include <utility>
#include <tl/expected.hpp>

struct Error {
    String message;
    int code = 0;

    Error() = default;
    Error(const char* msg, int c = 0) : message(msg), code(c) {}
    Error(String msg, int c = 0) : message(std::move(msg)), code(c) {}
};

template <typename T, typename E = Error>
using Result = tl::expected<T, E>;
