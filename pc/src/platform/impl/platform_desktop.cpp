#include "momentum/platform/impl/platform_desktop.hpp"

#include "momentum/platform/common/emergency_exit.hpp"
#include "momentum/platform/common/sdl.hpp"
#include "momentum/core/config/config.hpp"
#include "momentum/core/input/setup.hpp"
#include "momentum/utility/log.hpp"

#include <csignal>
#include <cstring>

#include <SDL3/SDL.h>

FILENAME_LOGGER();

namespace momentum::platform::impl {
    bool PlatformDesktop::Initialize() {
        InstallCrashHandler();
        InstallEmergencyExit();
        momentum::platform::common::sdl::InitializeSDLMeta();
        OnInitialized();
        return true;
    }

    RFRendererConfig PlatformDesktop::CreateRainfallConfig(std::string& cacheDir, std::string& backend) {
        RFRendererConfig config;
        memset(&config, 0, sizeof(config));
        config.shaderCacheDir = cacheDir.c_str();
        config.gameWidth      = 640;
        config.gameHeight     = 480;
        config.windowWidth    = Config::display.window_width;
        config.windowHeight   = Config::display.window_height;
        config.renderScale    = Config::display.render_scale;
        config.windowMode     = Config::display.window_mode;
        config.windowTitle    = APP_NAME;

        config.getClearColor  = [](float* r, float* g, float* b, float* a) {
            *r = 0.0f;
            *g = 0.0f;
            *b = 0.0f;
            *a = 1.0f;
        };

        config.onEvent = momentum::input::ProcessEvent;

        config.backendChoice = RF_BACKEND_CHOICE_AUTO;
        if (backend == "opengl") {
            config.backendChoice = RF_BACKEND_CHOICE_OPENGL;
        } else if (backend == "sdl3gpu") {
            config.backendChoice = RF_BACKEND_CHOICE_SDL3GPU;
            if (backend != "auto") {
                SDL_SetHint(SDL_HINT_GPU_DRIVER, backend.c_str());
                LOG_INFO("[GPU] Requesting backend: {}", backend);
            }
#if defined(__APPLE__)
		    else {
			    SDL_SetHint(SDL_HINT_GPU_DRIVER, "metal");
		    }
#endif
            SDL_SetHint(SDL_HINT_HIDAPI_IGNORE_DEVICES, "0x1e71/0x0000");
        } else if (backend == "auto") {
            config.backendChoice = RF_BACKEND_CHOICE_AUTO;
        } else {
            LOG_INFO("[GPU] Unknown backend: {}", backend);
        }
        return config;
    }

    void PlatformDesktop::InstallEmergencyExit() {
        std::signal(SIGINT, momentum::platform::common::emergency_exit::HandleEmergencyExit);
#ifdef SIGTERM
        std::signal(SIGTERM, momentum::platform::common::emergency_exit::HandleEmergencyExit);
#endif
    }

    void PlatformDesktop::InstallCrashHandler() {
    }

    void PlatformDesktop::WriteSdlGameState(FILE* log) {
        fprintf(log, "\nGame State:\n");
        const char* gpuDriver = SDL_GetHint(SDL_HINT_GPU_DRIVER);
        if (gpuDriver) {
            fprintf(log, "  GPU Driver: %s\n", gpuDriver);
        }

        fprintf(log, "  SDL Version: %d.%d.%d\n",
                SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    }

    uint64_t PlatformDesktop::GetTickNS() {
        return static_cast<uint64_t>(SDL_GetTicksNS());
    }

    void PlatformDesktop::DelayNS(uint64_t ns) {
        SDL_DelayNS(static_cast<Uint64>(ns));
    }

    uint64_t PlatformDesktop::GetTicks() {
        return static_cast<uint64_t>(SDL_GetTicks());
    }

    void PlatformDesktop::Delay(uint32_t ms) {
        SDL_Delay(ms);
    }

    void PlatformDesktop::PumpEvents() {
        SDL_PumpEvents();
    }

    bool PlatformDesktop::InitVideo() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            LOG_INFO("SDL_Init failed: {}", SDL_GetError());
            return false;
        }
        return true;
    }

    void PlatformDesktop::ShowOpenFileDialog(FileDialogCallback callback, void* userdata, void* window, const FileDialogFilter* filters, int nfilters, const char* default_location, bool allow_many) {
        SDL_ShowOpenFileDialog(
            reinterpret_cast<SDL_DialogFileCallback>(callback),
            userdata,
            static_cast<SDL_Window*>(window),
            reinterpret_cast<const SDL_DialogFileFilter*>(filters),
            nfilters,
            default_location,
            allow_many
        );
    }

    void PlatformDesktop::ShowOpenFolderDialog(FileDialogCallback callback, void* userdata, void* window, const char* default_location, bool allow_many) {
        SDL_ShowOpenFolderDialog(
            reinterpret_cast<SDL_DialogFileCallback>(callback),
            userdata,
            static_cast<SDL_Window*>(window),
            default_location,
            allow_many
        );
    }

    bool PlatformDesktop::ShowSimpleMessageBox(MessageBoxFlags flags, const char* title, const char* message, void* window) {
        return SDL_ShowSimpleMessageBox(
            static_cast<Uint32>(flags),
            title,
            message,
            static_cast<SDL_Window*>(window)
        );
    }
}
