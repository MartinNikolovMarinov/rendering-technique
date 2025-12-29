#pragma once

#include "core_init.h"

struct Surface;
namespace TGA { struct TGAImage; }

void logErr_PltErrorCode(core::PltErrCode pltErrCode);
void logErr_ConvErrorCode(core::ConversionError convErrCode);

void logInfo_Surface(const Surface& surface);

void logInfo_TGAFile(TGA::TGAImage& file);
