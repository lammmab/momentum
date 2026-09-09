#pragma once

#include <string>
#include <cstdint>
#include "momentum/utility/castable.hpp"
#include "rainfall/render/rf/renderer.h"

namespace momentum::platform {
    class IPlatform : public momentum::utility::Castable {
    public:
        struct FileDialogFilter {
            const char* mName;
            const char* mPattern;
        };

        enum class MessageBoxFlags : uint32_t {
            MSG_ERROR = 0x00000010u,
            MSG_WARNING = 0x00000020u,
            MSG_INFORMATION = 0x00000040u,
            MSG_BUTTONS_LEFT_TO_RIGHT = 0x00000080u,
            MSG_BUTTONS_RIGHT_TO_LEFT = 0x00000100u
        };

        using FileDialogCallback = void (*)(void* userdata, const char* const* filelist, int filter);

        virtual ~IPlatform() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual std::string GetName() const = 0;

        virtual RFRendererConfig CreateRainfallConfig(std::string& cacheDir, std::string& backend) = 0;

        virtual uint64_t GetTickNS() = 0;
        virtual void DelayNS(uint64_t ns) = 0;
        virtual uint64_t GetTicks() = 0;
        virtual void Delay(uint32_t ms) = 0;
        virtual void PumpEvents() = 0;

        virtual bool InitVideo() = 0;

        virtual void ShowOpenFileDialog(FileDialogCallback callback, void* userdata, void* window, const FileDialogFilter* filters, int nfilters, const char* default_location, bool allow_many) = 0;
        virtual void ShowOpenFolderDialog(FileDialogCallback callback, void* userdata, void* window, const char* default_location, bool allow_many) = 0;
        virtual bool ShowSimpleMessageBox(MessageBoxFlags flags, const char* title, const char* message, void* window) = 0;
    };
}
