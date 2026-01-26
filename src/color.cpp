#include "color.h"

Color randomColor() {
    Color color;
    color.rgba.r = u8(core::rndU32() % 255);
    color.rgba.g = u8(core::rndU32() % 255);
    color.rgba.b = u8(core::rndU32() % 255);
    color.rgba.a = 255;
    return color;
}
