#include "momentum/platform/common/crash_handler.hpp"

#include "momentum/core/config/config.hpp"
#include "momentum/core/paths.hpp"
#include "momentum/cr.hpp"

#include <filesystem>

namespace momentum::platform::common {
    char crash_handler::sLogRing[LOG_RING_SIZE][LOG_LINE_LEN];
    int  crash_handler::sLogRingPos   = 0;
    int  crash_handler::sLogRingCount = 0;
    bool crash_handler::sLogLockInit  = false;

    std::string crash_handler::GetCrashDumpDirectory() {
        std::filesystem::path dumpDir = Paths::UserDirFolder(Config::paths.crash_dumps);
        Paths::EnsureDirectory(dumpDir);
        return dumpDir.string();
    }

    std::string crash_handler::GetConfigPath() {
        namespace fs = std::filesystem;
        static const fs::path configPath = []() {
            fs::path launch = fs::path(Paths::GetLaunchDir() + CONFIG_FILENAME);
            std::error_code ec;
            if (fs::exists(launch, ec)) {
                return launch;
            }
            return fs::path(Paths::GetWriteDir() + CONFIG_FILENAME);
        }();
        return configPath.string();
    }

    void crash_handler::WriteLogRing(FILE* log) {
        fprintf(log, "\nConsole Output (last %d lines):\n", sLogRingCount);
        int start = (sLogRingCount < LOG_RING_SIZE) ? 0 : sLogRingPos;
        for (int i = 0; i < sLogRingCount; i++) {
            int idx = (start + i) % LOG_RING_SIZE;
            if (sLogRing[idx][0])
                fprintf(log, "  %s\n", sLogRing[idx]);
        }
    }
}