#ifdef _WIN32

#pragma once

#include "momentum/platform/impl/platform_desktop.hpp"

#include <cstdio>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace momentum::platform::impl {
    class PlatformWindows : public PlatformDesktop {
    public:
        virtual ~PlatformWindows() = default;

        void Shutdown() override;
        std::string GetName() const override;

    protected:
        void InstallEmergencyExit() override;
        void InstallCrashHandler() override;

    private:
        static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep);
        static const char* ExceptionCodeToString(DWORD code);
        static void WriteGameState(FILE* log);
        static void WriteSystemInfo(FILE* log);
    };
}

#endif // _WIN32