#include "cr/core/paths.hpp"
#include "cr/core/config/config.hpp"
#include <rainfall/platform.h>

#ifdef _WIN32
#include "cr/core/platform/windows.hpp"
FILE* pc_fopen(const char* path, const char* mode) {
    wchar_t wpath[1024];
    wchar_t wmode[16];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 1024);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 16);
    return _wfopen(wpath, wmode);
}
#else
FILE* pc_fopen(const char* path, const char* mode) {
    return fopen(path, mode);
}
#endif

namespace fs = std::filesystem;

bool Paths::IsDirectoryWritable(const fs::path& path) {
    fs::path testFile = path / ".cr_test_write";
    std::ofstream f(testFile);
    if (!f.is_open()) return false;
    f.close();
    fs::remove(testFile);
    return true;
}

void Paths::EnsureDirectory(const fs::path& path) {
    fs::create_directories(path);
}

bool Paths::FileExists(const fs::path& path) {
    return fs::is_regular_file(path);
}

bool Paths::DirectoryExists(const fs::path& path) {
   return fs::is_directory(path);
}

const std::string& Paths::GetLaunchDir() {
    static std::string launchPath = [] {
        const char* base = rfPlatformGetBasePath();
        std::string exeDir = base ? base : "./";
        rfPlatformFree(base);
        return exeDir;
    }();

    return launchPath;
}

const std::string& Paths::GetWriteDir() {
    static const std::string writePath = [] {
#if PLATFORM_IOS
        const char* home = getenv("HOME");
        if (home) return std::string(home) + "/Documents/";
#endif
        const std::string& exeDir = Paths::GetLaunchDir();
        const char* pref = rfPlatformGetPrefPath("Linifadomra", "CourageReborn");
        std::string result = pref ? pref : exeDir;
        rfPlatformFree(pref);
        return result;
    }();

    return writePath;
}

fs::path Paths::GetUserDir() {
    if (Config::paths.data_path.value == "") {
        return Paths::GetWriteDir();
    }
    return Config::paths.data_path.value;
}

fs::path Paths::UserDirFolder(const std::string& foldername) {
    return Paths::GetUserDir() / foldername;
}
