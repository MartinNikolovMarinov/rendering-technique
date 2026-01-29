#pragma once

#include "core_init.h"

struct Face3i {
    i32 data[3] = {};
    constexpr i32& operator[](i32 idx) { return data[idx]; }
};
