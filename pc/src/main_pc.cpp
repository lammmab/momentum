#ifdef _WIN32
#include "momentum/core/platform/windows.hpp"
#else
#include <unistd.h>
#include <cstdio>
#endif

#include "rainfall/render/rf/renderer.h"

#include "momentum/boot/arguments.hpp"
#include "momentum/utility/log.hpp"
FILENAME_LOGGER();

#include "momentum/momentum.hpp"
//#include "momentum/core/dol/assets.hpp"
#include "momentum/core/paths.hpp"

#include "dolphin/dvd.h"
#include "dolphin/vi/vifuncs.h"

#include "momentum/platform/platform_factory.hpp"
#include "momentum/core/service_locator.hpp"

#include "melee/gm/gmmain.h"

void run(std::unique_ptr<cr::platform::IPlatform>& platform) {
    melee_init();

    constexpr uint64_t FRAME_NS = 1000000000ULL / 30;
    uint64_t lastTickNs = platform->GetTickNS();

    while (rfRendererProcessEvents()) {
        uint64_t frameStart = platform->GetTickNS();

        /*float dt = (float)((double)(frameStart - lastTickNs) / 1.0e9);
        lastTickNs = frameStart;
        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.25f) dt = 0.25f;*/

        cr::input::Tick();

        VIWaitForRetrace();
        rfRendererBeginFrame();
        melee_frame();
        rfRendererEndFrame();

        uint64_t elapsed = platform->GetTickNS() - frameStart;

        if (elapsed < FRAME_NS) {
            platform->DelayNS(FRAME_NS - elapsed);
        }
    }

    rfRendererShutdown();
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    LOG_INFO("Starting Courage Reborn...");

#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    std::unique_ptr<cr::platform::IPlatform> platform = cr::platform::CreatePlatform();
    if (!platform) {
        LOG_ERROR("Failed to create platform, aborting...");
        return 1;
    }
    cr::core::service_locator::ProvidePlatform(platform.get());

    if (!platform->Initialize()) {
        LOG_ERROR("Failed to initialize platform {}, aborting...", platform->GetName().c_str());
        return 1;
    }

    cr::args::Config config = cr::args::parse(argc, argv);

    if (!Config::ConfigExists()) {
        Config::WriteConfig();
    } else {
        Config::ParseConfig();
    }

    if (!cr::assets::setup(config.disc_location)) {
        LOG_ERROR("Asset setup failed.");
        exit(1);
    }

    std::string gameData = Paths::UserDirFolder(Config::paths.game_folder).string();
    DVDSetRoot(gameData.c_str());

    /*if (!cr::dol::init(gameData.c_str())) {
        LOG_INFO("DOL assets failed to initialize..?");
        exit(1);
    }*/

    std::string shader_cache = Paths::UserDirFolder(Config::paths.shader_cache).string();
    LOG_INFO("Shader cache: {}", shader_cache);
    RFRendererConfig rfConfig = platform->CreateRainfallConfig(shader_cache, config.backend);

	if (!rfRendererInit(&rfConfig)) {
		exit(1);
	}

    cr::input::Initialize();
    cr::rf::initCard();

    run(platform);

    platform->Shutdown();

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return main(__argc, __argv);
}
#endif
