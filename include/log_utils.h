#pragma once

#include "core_init.h"

struct Surface;
namespace TGA { struct TGAImage; }

void logErr_PltErrorCode(core::PltErrCode pltErrCode);
void logErr_ConvErrorCode(core::ConversionError convErrCode);

void logInfo_Vector(core::vec4f v);
void logInfo_Vector(core::vec3f v);
void logInfo_Vector(core::vec2f v);
void logInfo_Vector(core::vec1f v);
void logInfo_Vector(core::vec4d v);
void logInfo_Vector(core::vec3d v);
void logInfo_Vector(core::vec2d v);
void logInfo_Vector(core::vec1d v);

void logInfo_Surface(const Surface& surface);

void logInfo_TGAFile(TGA::TGAImage& file);
