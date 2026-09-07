#pragma once

#include "momentum/platform/impl/platform_desktop.hpp"

#include <csignal>
#include <cstdio>
#include <string>

namespace momentum::platform::impl {
    class PlatformPosix : public PlatformDesktop {
    public:
        virtual ~PlatformPosix() = default;

        void Shutdown() override;
        std::string GetName() const override;

    protected:
        void InstallCrashHandler() override;
        void OnInitialized() override;

    private:
        static void CrashHandlerPosix(int sig, siginfo_t* info, void* ctx);
        static const char* SignalToString(int sig);
        static void WriteRegisters(FILE* log, void* ctx);
        static void WriteGameState(FILE* log);
        static void WriteSystemInfo(FILE* log);
        static void WriteConfig(FILE* log);
    };
}
