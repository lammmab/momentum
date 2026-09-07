#include "cr/boot/assets/setup.hpp"
#include "cr/boot/assets/asset_factory.hpp"

#include "cr/core/config/config.hpp"
#include "cr/core/paths.hpp"
#include "cr/core/service_locator.hpp"
#include "cr/platform/iplatform.hpp"
#include "cr/utility/log.hpp"
#include "cr/utility/region_format.hpp"

#include "gcesfa/gcesfa.hpp"
#include "gcesfa/logger.hpp"
#include "gcesfa/progress.hpp"
#include "gcesfa/search/search.hpp"

#include <siphon.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

CR_FILENAME_LOGGER();

esfa::interface::Registry cr::assets::registry{};

namespace {

constexpr int kSwapSchemaVersion = gcesfa::kSwapSchemaVersion;

bool dataExists() {
    const std::filesystem::path dataPath =
        Paths::UserDirFolder(Config::paths.game_folder);

    if (!std::filesystem::exists(dataPath)) {
        return false;
    }

    return std::filesystem::directory_iterator(dataPath) !=
           std::filesystem::directory_iterator();
}

bool siphonExtract(const char* path, const std::string& root, cr::platform::IPlatform& platform) {
    LOG_INFO("Extracting GameCube disc image: {}", path);
    SiphonError err = siphon_disc_extract(
        path, root.c_str(), REGIONS, NUM_REGIONS,
        [](void*, const char* msg) { LOG_INFO("[siphon] {}", msg); }, nullptr);

    if (err != SIPHON_OK) {
        LOG_INFO("Disc extraction failed with code {}", (int)err);
        platform.ShowSimpleMessageBox(
            cr::platform::IPlatform::MessageBoxFlags::MSG_ERROR,
            "Extraction Failed",
            "Failed to extract the game disc image.",
            nullptr);
        return false;
    }

    LOG_INFO("Asset extraction complete.");
    return true;
}

void gcesfaLog(gcesfa::LogLevel level, std::string_view msg) {
    switch (level) {
        case gcesfa::LogLevel::Debug:
            if (Config::meta.verbose_logging.value) {
                LOG_DEBUG("{}", msg);
            }
            break;
        case gcesfa::LogLevel::Info:
            if (Config::meta.verbose_logging.value) {
                LOG_INFO("{}", msg);
            }
            break;
        case gcesfa::LogLevel::Warning:
            LOG_WARN("{}", msg);
            break;
        case gcesfa::LogLevel::Error:
            LOG_ERROR("{}", msg);
            break;
    }
}

std::filesystem::path schemaFile(const std::filesystem::path& dir) {
    return dir / ".gcesfa_schema";
}

int readSwapSchema(const std::filesystem::path& dir) {
    std::ifstream in(schemaFile(dir));
    int version = 0;
    if (in) {
        in >> version;
    }
    return version;
}

void writeSwapSchema(const std::filesystem::path& dir) {
    std::ofstream out(schemaFile(dir), std::ios::trunc);
    if (out) {
        out << kSwapSchemaVersion << '\n';
    } else {
        LOG_ERROR("Failed to write swap schema file: {}", schemaFile(dir).string());
    }
}

bool swapSchemaCurrent(const std::filesystem::path& dir) {
    if (gcesfa::IsSwapped(dir).status == gcesfa::SwapStatus::NotStarted) {
        return true;
    }
    return readSwapSchema(dir) == kSwapSchemaVersion;
}

std::filesystem::path masterDir() {
    return Paths::UserDirFolder(Config::paths.game_folder.value + "_master");
}

bool restoreFromMaster(const std::filesystem::path& dir) {
    const auto master = masterDir();
    std::error_code ec;
    if (!std::filesystem::exists(master, ec)) return false;
    if (std::filesystem::directory_iterator(master, ec) == std::filesystem::directory_iterator()) return false;

    LOG_INFO("Restoring game data from pristine copy at {}", master.string());
    std::filesystem::copy(master, dir, std::filesystem::copy_options::recursive, ec);
    if (ec) {
        LOG_ERROR("Failed to restore from pristine copy: {}", ec.message());
        return false;
    }
    return true;
}

void saveMaster(const std::filesystem::path& dir) {
    const auto master = masterDir();
    std::error_code ec;
    if (std::filesystem::exists(master, ec)) return;

    LOG_INFO("Saving pristine copy of extracted game data to {}", master.string());
    std::filesystem::copy(dir, master, std::filesystem::copy_options::recursive, ec);
    if (ec) LOG_ERROR("Failed to save pristine copy: {}", ec.message());
}

bool runGCESFA(const std::filesystem::path& dir) {
    gcesfa::setLogger(gcesfaLog);
    gcesfa::RegisterDefaultFormats(cr::assets::registry);

    gcesfa::search::SearchConfig searchConfig;
    searchConfig.match = [](const std::filesystem::path&) -> gcesfa::search::matchResult {
        return {false, ""};
    };

    auto catalog = gcesfa::BuildAssetCatalog(cr::assets::registry, dir, searchConfig);

    try {
        gcesfa::SwapAssets(cr::assets::registry, dir, catalog);
        return true;
    } catch (const std::exception& e) {
        const auto progress = gcesfa::IsSwapped(dir);
        LOG_ERROR("gcesfa swap failed on '{}': {}",
            progress.failedOn.empty() ? "<unknown>" : progress.failedOn.string(),
            e.what());
        return false;
    } catch (...) {
        const auto progress = gcesfa::IsSwapped(dir);
        LOG_ERROR("gcesfa swap failed on '{}': unknown error (not derived from std::exception)",
            progress.failedOn.empty() ? "<unknown>" : progress.failedOn.string());
        return false;
    }
}

} // namespace

bool cr::boot::assets::IsValidDiscImage(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return false;
    }

    char id[6];
    const size_t r = fread(id, 1, sizeof(id), f);
    fclose(f);
    if (r < sizeof(id)) {
        return false;
    }

    for (auto& magic : IMAGE_FORMATS) {
        if (memcmp(id, magic.data(), magic.size()) == 0) {
            return true;
        }
    }
    for (auto& discId : RAW_DISC_IDS) {
        if (memcmp(id, discId.data(), discId.size()) == 0) {
            return true;
        }
    }
    return false;
}

bool cr::assets::setup(const std::string& disc_location) {
    cr::platform::IPlatform* platform = cr::core::service_locator::GetPlatform();
    if (!platform) {
        LOG_ERROR("Asset setup requires a platform.");
        return false;
    }

    auto source = cr::boot::assets::CreateAssetSource(*platform);
    if (!source->EnsureDataDirectory()) {
        return false;
    }

    const auto extractRoot = Paths::UserDirFolder(Config::paths.game_folder);

    if (dataExists()) {
        if (swapSchemaCurrent(extractRoot)) {
            LOG_INFO("Game data found at: {}", extractRoot.string());
            writeSwapSchema(extractRoot);
            if (!runGCESFA(extractRoot)) return false;
            Config::WriteConfig();
            return true;
        }

        LOG_INFO("Game data at {} was converted by an older gcesfa "
                 "(schema {} vs current {}); reconverting from pristine data",
                 extractRoot.string(), readSwapSchema(extractRoot), kSwapSchemaVersion);

        std::error_code ec;
        std::filesystem::remove_all(extractRoot, ec);
        if (ec) {
            LOG_ERROR("Failed to remove stale game data at {}: {}",
                      extractRoot.string(), ec.message());
            return false;
        }
    }

    if (restoreFromMaster(extractRoot)) {
        LOG_INFO("Game data found at: {}", extractRoot.string());
        writeSwapSchema(extractRoot);
        if (!runGCESFA(extractRoot)) return false;
        Config::WriteConfig();
        return true;
    }

    std::string isoPath = disc_location;
    if (isoPath.empty()) {
        if (!source->AcquireDiscImage(isoPath)) {
            return false;
        }
    } else if (!cr::boot::assets::IsValidDiscImage(isoPath.c_str())) {
        LOG_INFO("Invalid disc location: {}", isoPath);
        return false;
    }

    if (!siphonExtract(isoPath.c_str(), extractRoot.string(), *platform)) {
        return false;
    }

    saveMaster(extractRoot);

    writeSwapSchema(extractRoot);
    if (!runGCESFA(extractRoot)) {
        return false;
    }

    Config::WriteConfig();
    return true;
}
