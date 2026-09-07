#include "cr/core/config/config.hpp"

#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "cr/core/config/input/input_config.hpp"
#include "cr/core/paths.hpp"
#include "cr/utility/log.hpp"
#include "cr/core/service_locator.hpp"

CR_FILENAME_LOGGER();

MetaConfig Config::meta;
PathConfig Config::paths;
EnhancementConfig Config::enhancements;
DisplayConfig Config::display;
InputConfig Config::input;
GameConfig Config::game;
AudioConfig Config::audio;
CosmeticsConfig Config::cosmetics;

static const std::string LAUNCH_DIR = [] {
#ifndef _WIN32
    return Paths::GetWriteDir();
#else
    return Paths::GetLaunchDir();
#endif
}();

/* Mode Helpers */
DisplayMode Config::strDisplayMode(const char* displayMode) {
    if (strcmp(displayMode, "4:3")  == 0) return DisplayMode::STANDARD;
    if (strcmp(displayMode, "16:9") == 0) return DisplayMode::WIDESCREEN;
    if (strcmp(displayMode, "21:9") == 0) return DisplayMode::ULTRAWIDE;
    throw std::runtime_error(std::string("Invalid display mode: '") + displayMode + "'");
}

WindowMode Config::strWindowMode(const char* windowMode) {
    if (strcmp(windowMode, "windowed")   == 0) return WindowMode::WINDOWED;
    if (strcmp(windowMode, "borderless") == 0) return WindowMode::WINDOWED_BORDERLESS;
    if (strcmp(windowMode, "fullscreen") == 0) return WindowMode::FULLSCREEN;
    throw std::runtime_error(std::string("Invalid window mode: '") + windowMode + "'");
}

const char* Config::fmtDisplayMode(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::STANDARD:  return "4:3";
        case DisplayMode::WIDESCREEN: return "16:9";
        case DisplayMode::ULTRAWIDE: return "21:9";
        default: throw std::runtime_error("Invalid DisplayMode value");
    }
}

const char* Config::fmtWindowMode(WindowMode mode) {
    switch (mode) {
        case WindowMode::WINDOWED:            return "windowed";
        case WindowMode::WINDOWED_BORDERLESS: return "borderless";
        case WindowMode::FULLSCREEN:          return "fullscreen";
        default: throw std::runtime_error("Invalid WindowMode value");
    }
}

/* INI Helpers */
bool Config::ConfigExists() {
    return Paths::FileExists(LAUNCH_DIR + CONFIG_FILENAME);
}

const void Config::ParseConfig() {
    parseConfiguration(LAUNCH_DIR + CONFIG_FILENAME, meta, paths, display, enhancements, input, game,
                       audio, cosmetics);
}

const void Config::WriteConfig() {
    std::string path = LAUNCH_DIR + CONFIG_FILENAME;
    writeConfiguration(path, meta, paths, display, enhancements, input, game, audio, cosmetics);
#ifndef _WIN32
    int fd = open(path.c_str(), O_RDONLY);
    if (fd >= 0) {
        fsync(fd);
        close(fd);
    }
#endif
}

bool Config::parseColor(const std::string& str, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
    if (str == "default" || str.empty()) {
        return false;
    }
    
    // Normalize to lowercase
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);

    // Strip enclosing double or single quotes if present
    if (lowerStr.length() >= 2) {
        if ((lowerStr.front() == '"' && lowerStr.back() == '"') ||
            (lowerStr.front() == '\'' && lowerStr.back() == '\'')) {
            lowerStr = lowerStr.substr(1, lowerStr.length() - 2);
        }
    }

    if (lowerStr == "rainbow") {
        float hue = std::fmod((float)cr::core::service_locator::GetPlatform()->GetTicks() * 0.0003f, 1.0f);
        float h = hue * 6.0f;
        float x = 1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f);
        float r1 = 0, g1 = 0, b1 = 0;
        if (h < 1.0f) {
            r1 = 1.0f; g1 = x;
        } else if (h < 2.0f) {
            r1 = x; g1 = 1.0f;
        } else if (h < 3.0f) {
            g1 = 1.0f; b1 = x;
        } else if (h < 4.0f) {
            g1 = x; b1 = 1.0f;
        } else if (h < 5.0f) {
            r1 = x; b1 = 1.0f;
        } else {
            r1 = 1.0f; b1 = x;
        }
        r = (uint8_t)(r1 * 255.0f);
        g = (uint8_t)(g1 * 255.0f);
        b = (uint8_t)(b1 * 255.0f);
        a = 255;
        return true;
    }
    
    // Check named colors
    static const std::unordered_map<std::string, uint32_t> namedColors = {
        {"red",     0xFF0000FF},
        {"green",   0x00FF00FF},
        {"blue",    0x0000FFFF},
        {"yellow",  0xFFFF00FF},
        {"magenta", 0xFF00FFFF},
        {"cyan",    0x00FFFFFF},
        {"white",   0xFFFFFFFF},
        {"black",   0x000000FF},
        {"orange",  0xFFA500FF},
        {"pink",    0xFFC0CBFF},
        {"purple",  0x800080FF},
        {"brown",   0xA52A2AFF},
        {"grey",    0x808080FF},
        {"gray",    0x808080FF},
        {"gold",    0xFFD700FF},
        {"silver",  0xC0C0C0FF}
    };
    
    auto it = namedColors.find(lowerStr);
    if (it != namedColors.end()) {
        uint32_t val = it->second;
        r = (val >> 24) & 0xFF;
        g = (val >> 16) & 0xFF;
        b = (val >> 8) & 0xFF;
        a = val & 0xFF;
        return true;
    }
    
    std::string hex = lowerStr;
    if (hex[0] == '#') {
        hex = hex.substr(1);
    }
    
    if (hex.length() == 6) {
        unsigned int val;
        if (sscanf(hex.c_str(), "%x", &val) == 1) {
            r = (val >> 16) & 0xFF;
            g = (val >> 8) & 0xFF;
            b = val & 0xFF;
            a = 0xFF;
            return true;
        }
    } else if (hex.length() == 8) {
        unsigned int val;
        if (sscanf(hex.c_str(), "%x", &val) == 1) {
            r = (val >> 24) & 0xFF;
            g = (val >> 16) & 0xFF;
            b = (val >> 8) & 0xFF;
            a = val & 0xFF;
            return true;
        }
    }
    return false;
}
