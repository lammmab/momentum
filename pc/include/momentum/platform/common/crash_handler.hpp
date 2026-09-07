#pragma once

#include <string>
#include <cstdio>

#define LOG_RING_SIZE 256
#define LOG_LINE_LEN  256

namespace momentum::platform::common {
    namespace crash_handler {
        extern char sLogRing[LOG_RING_SIZE][LOG_LINE_LEN];
        extern int  sLogRingPos;
        extern int  sLogRingCount;
        extern bool sLogLockInit;

        std::string GetCrashDumpDirectory();
        std::string GetConfigPath();
        void WriteLogRing(FILE* log);
    }
}
