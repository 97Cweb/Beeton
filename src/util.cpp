#include "Beeton.h"
#include <vector>

void Beeton::logBeeton(BeetonLogLevel level, const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    switch(level) {
    case BEETON_LOG_DEBUG:
        log_d("[Beeton] %s", buffer);
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

std::vector<String> Beeton::splitCsv(const String &input) {
    std::vector<String> result;
    int start = 0;
    int end = 0;

    while((end = input.indexOf(',', start)) != -1) {
        result.push_back(input.substring(start, end));
        start = end + 1;
    }

    // Add the final part
    if(start < input.length()) {
        result.push_back(input.substring(start));
    }

    return result;
}

String Beeton::formatPayload(const std::vector<uint8_t> &payload) {
    String result;
    for(size_t i = 0; i < payload.size(); ++i) {
        if(i > 0)
            result += " ";
        result += String(payload[i], DEC);
    }
    return result;
}
