/*
 * config.h
 * Centralized location for all configuration
 * To add new enhancement:
 * 1. Add an entry to EnhancementConfig
 * 2. Add the entry to the entries() function
 */
#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include <rainfall/config/configuration.h>

#include "momentum/core/config/input/input_config.hpp"
#include "momentum/core/config/config_types.h"

/* Default Names */
constexpr static const char* GAME_TITLE = "Momentum";

constexpr static const char* MODS_FOLDERNAME  = "mods";
constexpr static const char* MOD_SAVES_FOLDERNAME  = "mod_saves";
constexpr static const char* GAME_FOLDERNAME  = "game_data";
constexpr static const char* LOGS_FOLDERNAME  = "logs";
constexpr static const char* CRASH_FOLDERNAME = "crash_dumps";
constexpr static const char* SHADER_FOLDERNAME  = "shader_cache";
constexpr static const char* SAVES_FOLDERNAME  = "saves";
constexpr static const char* CONFIG_FILENAME  = "config.ini";

/* Configurations */
constexpr static int META_VERSION = 1;
struct MetaConfig {
    const char* header = "meta";
    ConfigEntry<int> version = CONFIG_ENTRY("version", META_VERSION);

    auto entries() {
        return std::tie(version);
    }
};

// Toggled off config entry helper
static inline ConfigEntry<bool> CONFIG_ENTRY_BOOLEAN(std::string key) {
    return CONFIG_ENTRY(key, false);
}

/* Pathing */
struct PathConfig {
    const char* header = "paths";
    ConfigEntry<std::string> data_path        = CONFIG_ENTRY("data_path",   std::string("")); // This is NOT the same as game_data. game_data lives inside of this.
    ConfigEntry<std::string> game_folder      = CONFIG_ENTRY("game_folder", std::string(GAME_FOLDERNAME));
    ConfigEntry<std::string> log_dumps        = CONFIG_ENTRY("log_dumps", std::string(LOGS_FOLDERNAME));
    ConfigEntry<std::string> crash_dumps      = CONFIG_ENTRY("crash_dumps", std::string(CRASH_FOLDERNAME));
    ConfigEntry<std::string> shader_cache     = CONFIG_ENTRY("shader_cache", std::string(SHADER_FOLDERNAME));
    ConfigEntry<std::string> saves_folder     = CONFIG_ENTRY("saves_folder", std::string(SAVES_FOLDERNAME));
    ConfigEntry<std::string> mods_folder      = CONFIG_ENTRY("mods_folder", std::string(MODS_FOLDERNAME));
    ConfigEntry<std::string> mod_saves_folder = CONFIG_ENTRY("mod_saves_folder", std::string(MOD_SAVES_FOLDERNAME));


    auto entries() {
        return std::tie(
            data_path, 
            game_folder,
            log_dumps,
            crash_dumps,
            shader_cache,
            saves_folder
        );
    }
};

struct EnhancementConfig {
    const char* header = "enhancements";
    ConfigEntry<bool> skip_intro_video        = CONFIG_ENTRY_BOOLEAN("skip_intro_video");

    auto entries() {
        return std::tie(
            skip_intro_video
        );
    }
};

struct DisplayConfig {
    const char* header = "display";
    ConfigEntry<float> render_scale      = CONFIG_ENTRY("render_scale", 1.5f);
    ConfigEntry<int>   window_mode       = CONFIG_ENTRY("window_mode", (int)WINDOWED);
    ConfigEntry<int>   window_width      = CONFIG_ENTRY("window_width", 640);
    ConfigEntry<int>   window_height     = CONFIG_ENTRY("window_height", 480);

    ConfigEntry<int>   display_mode      = CONFIG_ENTRY("display_mode", (int)STANDARD);
    ConfigEntry<bool>  vsync             = CONFIG_ENTRY("vsync", true);
    ConfigEntry<bool>  stretch_thp_video = CONFIG_ENTRY_BOOLEAN("stretch_thp_video");

    auto entries() {
        return std::tie(
            render_scale,
            window_mode,
            display_mode,
            vsync,
            stretch_thp_video,
            window_width,
            window_height
        );
    }
};

/* Input delegated to pc_input_config.h */

class Config {
public:
    /* Config Entries */
    static PathConfig paths;
    static InputConfig input;
    static EnhancementConfig enhancements;
    static DisplayConfig display;
    static MetaConfig meta;

    /* INI Helpers */
    static bool configExists();
    static const void parseConfig();
    static const void writeConfig();
    static const std::string& configFilePath();
};

#endif
