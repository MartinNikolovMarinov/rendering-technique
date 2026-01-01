#pragma once

#include "core_init.h"

struct Surface;
namespace TGA { struct TGAImage; }

void logErr_PltErrorCode(core::PltErrCode pltErrCode);
void logErr_ConvErrorCode(core::ConversionError convErrCode);

void logDirect_Vector(core::vec4f v);
void logDirect_Vector(core::vec3f v);
void logDirect_Vector(core::vec2f v);
void logDirect_Vector(core::vec1f v);
void logDirect_Vector(core::vec4d v);
void logDirect_Vector(core::vec3d v);
void logDirect_Vector(core::vec2d v);
void logDirect_Vector(core::vec1d v);
void logDirect_Vector(core::vec4i v);
void logDirect_Vector(core::vec3i v);
void logDirect_Vector(core::vec2i v);
void logDirect_Vector(core::vec1i v);

void logInfo_Surface(const Surface& surface);

void logInfo_TGAFile(TGA::TGAImage& file);
