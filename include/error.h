#pragma once

#include "core_init.h"

enum struct Error {
    Unknown = 0,

    FailedToCreateSurface,

    SENTINEL
};

constexpr const char* errorToCstr(Error err) {
    switch (err)
    {
        case Error::FailedToCreateSurface: return "Failed to create surface";

        case Error::Unknown:  [[fallthrough]];
        case Error::SENTINEL: [[fallthrough]];
        default:              return "unknown";
    }
}
