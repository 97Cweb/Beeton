#include "Beeton.h"

void Beeton::logBeeton(BeetonLogLevel level, const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    switch (level) {
        case BEETON_LOG_VERBOSE:
            log_v("[Beeton] %s", buffer);
            break;
        case BEETON_LOG_INFO:
            log_i("[Beeton] %s", buffer);
            break;
        case BEETON_LOG_WARN:
            log_w("[Beeton] %s", buffer);
            break;
        case BEETON_LOG_ERROR:
            log_e("[Beeton] %s", buffer);
            break;
    }
}
