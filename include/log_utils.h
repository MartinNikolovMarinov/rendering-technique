#include "core_init.h"

struct Surface;
namespace TGA { struct TGAFile; }

void logErr_PltErrorCode(core::PltErrCode pltErrCode);

void logInfo_Surface(const Surface& surface);

void logInfo_TGAFile(TGA::TGAFile& file);
