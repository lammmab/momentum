#include "cr/boot/assets/impl/asset_source_desktop.hpp"

#include "cr/boot/assets/setup.hpp"
#include "cr/core/config/config.hpp"
#include "cr/utility/log.hpp"

#include <atomic>
#include <cstdio>
#include <string>

CR_FILENAME_LOGGER();

namespace {

constexpr const char* kUnchosenDataDir = "";

struct FileDialogResult {
    char mPath[512]{};
    std::atomic<bool> mDone{false};
    bool mCancelled = false;
};

void fileDialogCallback(void* userdata, const char* const* filelist, int /*filter*/) {
    auto* result = static_cast<FileDialogResult*>(userdata);
    result->mCancelled = (filelist == nullptr || filelist[0] == nullptr);

    if (!result->mCancelled) {
        std::snprintf(result->mPath, sizeof(result->mPath), "%s", filelist[0]);
    }

    result->mDone.store(true, std::memory_order_release);
}

bool waitForDialog(cr::platform::IPlatform& platform, FileDialogResult& result) {
    while (!result.mDone.load(std::memory_order_acquire)) {
        platform.PumpEvents();
        platform.Delay(10);
    }
    return !result.mCancelled && result.mPath[0] != '\0';
}

} // namespace

namespace cr::boot::assets::impl {
    AssetSourceDesktop::AssetSourceDesktop(cr::platform::IPlatform& platform)
        : mPlatform(platform) {}

    bool AssetSourceDesktop::EnsureDataDirectory() {
        if (Config::paths.data_path.value != kUnchosenDataDir) {
            return true;
        }

        if (!mPlatform.InitVideo()) {
            return false;
        }

        mPlatform.ShowSimpleMessageBox(
            cr::platform::IPlatform::MessageBoxFlags::MSG_INFORMATION,
            GAME_TITLE,
            "Data directory not chosen.\n\n"
            "Please select your desired data directory. This is where your extracted game disc, mods, crash dumps, logs, and all data will live!",
            nullptr);

        FileDialogResult result{};
        mPlatform.ShowOpenFolderDialog(fileDialogCallback, &result, nullptr, nullptr, false);

        if (!waitForDialog(mPlatform, result)) {
            LOG_INFO("User cancelled folder selection.");
            return false;
        }

        Config::paths.data_path.value = result.mPath;
        return true;
    }

    bool AssetSourceDesktop::AcquireDiscImage(std::string& outPath) {
        if (!mPlatform.InitVideo()) {
            return false;
        }

        mPlatform.ShowSimpleMessageBox(
            cr::platform::IPlatform::MessageBoxFlags::MSG_INFORMATION,
            GAME_TITLE,
            "Game assets not found.\n\n"
            "Please select your Twilight Princess GameCube disc file.",
            nullptr);

        cr::platform::IPlatform::FileDialogFilter filters[1];
        filters[0].mName = "GameCube Disc Image";
        filters[0].mPattern = "iso;gcm;ciso;gcz;wia;rvz;wbfs;nfs;tgc";

        char selectedPath[512]{};
        while (true) {
            FileDialogResult result{};
            mPlatform.ShowOpenFileDialog(
                fileDialogCallback, &result, nullptr, filters, 1, nullptr, false);

            if (!waitForDialog(mPlatform, result)) {
                LOG_INFO("User cancelled file selection.");
                return false;
            }

            std::snprintf(selectedPath, sizeof(selectedPath), "%s", result.mPath);
            if (IsValidDiscImage(selectedPath)) {
                outPath = selectedPath;
                return true;
            }

            mPlatform.ShowSimpleMessageBox(
                cr::platform::IPlatform::MessageBoxFlags::MSG_ERROR,
                "Invalid ISO",
                "Not a recognized Twilight Princess disc image.\n"
                "Supported formats: ISO, CISO, GCZ, WIA, RVZ, WBFS, NFS, TGC\n\n"
                "Please select the correct file.",
                nullptr);
        }
    }
}
