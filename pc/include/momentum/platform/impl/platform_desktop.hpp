#pragma once

#include "momentum/platform/iplatform.hpp"

#include <cstdio>
#include <string>

namespace momentum::platform::impl {
class PlatformDesktop : public momentum::platform::IPlatform {
public:
    virtual ~PlatformDesktop() = default;

    bool Initialize() override;

    RFRendererConfig CreateRainfallConfig(std::string& cacheDir, std::string& backend) override;

    uint64_t GetTickNS() override;
    void DelayNS(uint64_t ns) override;
    uint64_t GetTicks() override;
    void Delay(uint32_t ms) override;
    void PumpEvents() override;
    bool InitVideo() override;

    void ShowOpenFileDialog(FileDialogCallback callback, void* userdata, void* window,
                             const FileDialogFilter* filters, int nfilters,
                             const char* default_location, bool allow_many) override;
    void ShowOpenFolderDialog(FileDialogCallback callback, void* userdata, void* window,
                               const char* default_location, bool allow_many) override;
    bool ShowSimpleMessageBox(MessageBoxFlags flags, const char* title, const char* message, void* window) override;

protected:
    virtual void InstallEmergencyExit();
    virtual void InstallCrashHandler();

    virtual void OnInitialized() {}

    static void WriteSdlGameState(FILE* log);
};
}
