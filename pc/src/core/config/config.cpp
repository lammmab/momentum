#include "momentum/core/config/config.hpp"

#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "momentum/core/config/input/input_config.hpp"
#include "momentum/core/paths.hpp"
#include "momentum/utility/log.hpp"
#include "momentum/core/service_locator.hpp"

FILENAME_LOGGER();

MetaConfig Config::meta;
PathConfig Config::paths;
EnhancementConfig Config::enhancements;
DisplayConfig Config::display;
InputConfig Config::input;

static const std::string LAUNCH_DIR = [] {
#ifndef _WIN32
    return Paths::GetWriteDir();
#else
    return Paths::GetLaunchDir();
#endif
}();

/* Mode Helpers */
bool Config::configExists() {
    return Paths::FileExists(LAUNCH_DIR + CONFIG_FILENAME);
}

const void Config::parseConfig() {
    parseConfiguration(LAUNCH_DIR + CONFIG_FILENAME, meta, paths, display, enhancements, input);
}

const void Config::writeConfig() {
    std::string path = LAUNCH_DIR + CONFIG_FILENAME;
    writeConfiguration(path, meta, paths, display, enhancements, input);
#ifndef _WIN32
    int fd = open(path.c_str(), O_RDONLY);
    if (fd >= 0) {
        fsync(fd);
        close(fd);
    }
#endif
}
