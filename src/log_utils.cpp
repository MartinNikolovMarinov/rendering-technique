#include "log_utils.h"
#include "surface.h"

void logErr_PltErrorCode(core::PltErrCode pltErrCode) {
    char buff[core::MAX_SYSTEM_ERR_MSG_SIZE] = {};
    bool ok = core::pltErrorDescribe(pltErrCode, buff);
    if (ok) {
        logErr("Platform Error; reason: {}", buff);
    }
    else {
        logErr("Platform Error; failed to describe error code: {}", i64(pltErrCode));
    }
}

void logInfo_Surface(const Surface& surface) {
    logInfo(
        "Surface: width={}, height={}, pitch={}",
        surface.width, surface.height, surface.pitch
    );

    // TODO: log the pixel format appropriately: surface.pixelFormat
}
