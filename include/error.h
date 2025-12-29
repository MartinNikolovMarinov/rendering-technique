#pragma once

#include "core_init.h"

enum struct Error {
    Unknown = 0,

    SENTINEL
};

constexpr const char* errorToCstr(Error err) {
    switch (err)
    {
        case Error::Unknown:  [[fallthrough]];
        case Error::SENTINEL: [[fallthrough]];
        default:              return "unknown";
    }
}
