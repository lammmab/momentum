#pragma once

#ifndef PATH_HELPERS_H
#define PATH_HELPERS_H

#include <filesystem>
#include <string>
#include <cstdio>

class Paths {
public:
    static std::filesystem::path UserDirFolder(const std::string& foldername);
    static std::filesystem::path GetUserDir();

    static const std::string& GetLaunchDir();
    static const std::string& GetWriteDir();

    static bool IsDirectoryWritable(const std::filesystem::path& path);
    static bool DirectoryExists(const std::filesystem::path& path);
    static bool FileExists(const std::filesystem::path& path);
    static void EnsureDirectory(const std::filesystem::path& path);
};

FILE* pc_fopen(const char* path, const char* mode);

#endif
