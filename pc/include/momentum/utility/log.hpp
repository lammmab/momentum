#pragma once

#ifndef LOG_H
#define LOG_H

#if defined(__SWITCH__)

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>

namespace log {

inline void appendFmt(std::string& out, const char* fmt);

inline void appendEscaped(std::string& out, const char* fmt) {
    if (!fmt)
        return;
    while (*fmt) {
        if (fmt[0] == '{' && fmt[1] == '{') {
            out.push_back('{');
            fmt += 2;
        } else if (fmt[0] == '}' && fmt[1] == '}') {
            out.push_back('}');
            fmt += 2;
        } else {
            out.push_back(*fmt++);
        }
    }
}

inline int parsePrecision(const char* spec, int fallback) {
    const char* dot = std::strchr(spec, '.');
    if (!dot)
        return fallback;

    int precision = 0;
    const char* p = dot + 1;
    while (*p >= '0' && *p <= '9') {
        precision = precision * 10 + (*p - '0');
        p++;
    }
    return precision;
}

inline int parseWidth(const char* spec, bool* zeroPad) {
    int width = 0;
    const char* p = spec;
    if (*p == ':')
        p++;
    if (*p == '0') {
        *zeroPad = true;
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }
    return width;
}

inline void appendString(std::string& out, const char* value) {
    out += value ? value : "(null)";
}

template<typename T>
inline void appendValue(std::string& out, const char* spec, T&& value) {
    using D = std::decay_t<T>;
    char buf[128];

    if constexpr (std::is_same_v<D, std::string>) {
        out += value;
    } else if constexpr (std::is_convertible_v<T, const char*>) {
        appendString(out, value);
    } else if constexpr (std::is_same_v<D, bool>) {
        out += value ? "true" : "false";
    } else if constexpr (std::is_floating_point_v<D>) {
        const int precision = parsePrecision(spec, 6);
        std::snprintf(buf, sizeof(buf), "%.*f", precision, static_cast<double>(value));
        out += buf;
    } else if constexpr (std::is_integral_v<D>) {
        const bool hexLower = std::strchr(spec, 'x') != nullptr;
        const bool hexUpper = std::strchr(spec, 'X') != nullptr;
        bool zeroPad = false;
        const int width = parseWidth(spec, &zeroPad);

        if (hexLower || hexUpper) {
            char fmt[16];
            std::snprintf(fmt, sizeof(fmt), "%%%s%dll%c",
                          zeroPad ? "0" : "", width, hexUpper ? 'X' : 'x');
            std::snprintf(buf, sizeof(buf), fmt, static_cast<unsigned long long>(value));
        } else {
            char fmt[16];
            std::snprintf(fmt, sizeof(fmt), "%%%s%d%s",
                          zeroPad ? "0" : "", width,
                          std::is_signed_v<D> ? "lld" : "llu");
            if constexpr (std::is_signed_v<D>)
                std::snprintf(buf, sizeof(buf), fmt, static_cast<long long>(value));
            else
                std::snprintf(buf, sizeof(buf), fmt, static_cast<unsigned long long>(value));
        }
        out += buf;
    } else if constexpr (std::is_pointer_v<D>) {
        std::snprintf(buf, sizeof(buf), "%p", static_cast<const void*>(value));
        out += buf;
    } else {
        out += "<?>";
    }
}

template<typename T, typename... Rest>
inline void appendFmt(std::string& out, const char* fmt, T&& value, Rest&&... rest) {
    if (!fmt)
        return;

    while (*fmt) {
        if (fmt[0] == '{' && fmt[1] == '{') {
            out.push_back('{');
            fmt += 2;
            continue;
        }
        if (fmt[0] == '}' && fmt[1] == '}') {
            out.push_back('}');
            fmt += 2;
            continue;
        }
        if (*fmt == '{') {
            const char* close = std::strchr(fmt + 1, '}');
            if (!close) {
                out.push_back(*fmt++);
                continue;
            }

            std::string spec(fmt + 1, close);
            appendValue(out, spec.c_str(), std::forward<T>(value));
            appendFmt(out, close + 1, std::forward<Rest>(rest)...);
            return;
        }

        out.push_back(*fmt++);
    }
}

inline void appendFmt(std::string& out, const char* fmt) {
    appendEscaped(out, fmt);
}

} // namespace crlog

template<typename... Args>
inline void crLogPrint(const char* prefix, const char* fmt, Args&&... args) {
    std::string msg;
    crlog::appendFmt(msg, fmt, std::forward<Args>(args)...);
    std::fprintf(stderr, "%s%s\n", prefix, msg.c_str());
    std::fflush(stderr);
}

#define FILENAME_LOGGER()
#define LOG_INFO(fmt, ...)  crLogPrint("[INFO] ",  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  crLogPrint("[WARN] ",  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) crLogPrint("[ERROR] ", fmt, ##__VA_ARGS__)

#define DEBUG_LOG_INFO(...)
#define DEBUG_LOG_WARN(...)
#define DEBUG_LOG_ERROR(...)
#define DEBUG_LOG_EVERY_FRAMES(INTERVAL, LOG_MACRO, ...)
#define FILE_LOG_INFO(...)
#define FILE_LOG_WARN(...)
#define FILE_LOG_ERROR(...)
#define FILE_LOG_EVERY_FRAMES(INTERVAL, LOG_MACRO, ...)

#else // !__SWITCH__

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>

#include "cr/core/paths.hpp"
#include "cr/core/config/config.hpp"

// Core macros: always active, write to the per-file console logger

#define LOG_INFO(...)  logger->info(__VA_ARGS__)
#define LOG_WARN(...)  logger->warn(__VA_ARGS__)
#define LOG_ERROR(...) logger->error(__VA_ARGS__)
#define LOG_DEBUG(...) logger->debug(__VA_ARGS__)

inline void InitAsyncLogging()
{
    static bool initialized = false;
    if (initialized) return;
    spdlog::init_thread_pool(8192, 1);
    initialized = true;
}

// File logger

inline std::shared_ptr<spdlog::logger> GetFileLogger()
{
    static auto instance = [] {
        InitAsyncLogging();
        const std::filesystem::path p = Paths::UserDirFolder(Config::paths.log_dumps);
        Paths::EnsureDirectory(p);
        printf("Dumping here: %s\n", p.string().c_str());
        return spdlog::rotating_logger_mt<spdlog::async_factory>(
            "file_logger", p.string(), 1024 * 1024 * 10, 3);
    }();
    return instance;
}

#define FILENAME_LOGGER() \
    static auto logger = [] { \
        InitAsyncLogging(); \
        return spdlog::stdout_color_mt<spdlog::async_factory>( \
            std::filesystem::path(__FILE__).filename().string()); \
    }()

// DEBUG_LOG_*: console only, stripped in release

#if defined(DEBUG_LOGGING)
#define DEBUG_LOG_INFO(...)  logger->info(__VA_ARGS__)
#define DEBUG_LOG_WARN(...)  logger->warn(__VA_ARGS__)
#define DEBUG_LOG_ERROR(...) logger->error(__VA_ARGS__)
#define DEBUG_LOG_EVERY_FRAMES(INTERVAL, LOG_MACRO, ...) \
    do { \
        static int _sLogThrottle = 0; \
        if ((_sLogThrottle++ % (INTERVAL)) == 0) { LOG_MACRO(__VA_ARGS__); } \
    } while (0)
#else
#define DEBUG_LOG_INFO(...)
#define DEBUG_LOG_WARN(...)
#define DEBUG_LOG_ERROR(...)
#define DEBUG_LOG_EVERY_FRAMES(INTERVAL, LOG_MACRO, ...)
#endif

// FILE_LOG_*: rotating file only, stripped in debug

#if !defined(DEBUG_LOGGING)
#define FILE_LOG_INFO(...)  GetFileLogger()->info(__VA_ARGS__)
#define FILE_LOG_WARN(...)  GetFileLogger()->warn(__VA_ARGS__)
#define FILE_LOG_ERROR(...) GetFileLogger()->error(__VA_ARGS__)
#define FILE_LOG_EVERY_FRAMES(INTERVAL, LOG_MACRO, ...) \
        do { \
            static int _sFileLogThrottle = 0; \
            if ((_sFileLogThrottle++ % (INTERVAL)) == 0) { LOG_MACRO(__VA_ARGS__); } \
        } while (0)
#else
#define FILE_LOG_INFO(...)
#define FILE_LOG_WARN(...)
#define FILE_LOG_ERROR(...)
#define FILE_LOG_EVERY_FRAMES(INTERVAL, LOG_MACRO, ...)
#endif

#endif // __SWITCH__

#endif // LOG_H
