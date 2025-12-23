#include "log_utils.h"

void logPltErrorCode(core::PltErrCode pltErrCode) {
    char buff[core::MAX_SYSTEM_ERR_MSG_SIZE] = {};
    bool ok = core::pltErrorDescribe(pltErrCode, buff);
    if (ok) {
        logErr("reason: {}", buff);
    }
    else {
        logErr("reason: Platform Layer failed to describe reason");
    }
}
