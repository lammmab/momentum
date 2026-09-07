#include "rainfall/mem/card.hpp"
#include "momentum/core/config/config.hpp"
#include "momentum/core/paths.hpp"
#include "momentum/utility/region_format.hpp"

#include "momentum/momentum.hpp"

#include <cstring>
#include <string>

namespace card = rainfall::card;

namespace {

const char* const kSaveExtension = ".gci";
constexpr u8 kCompany[2] = { '0', '1' };

constexpr size_t kSavedataSize = 0xA94;
constexpr size_t kDataSSize = 0x2000;
constexpr size_t kSector = 0x2000;

void swap16(u8* p) { u8 t = p[0]; p[0] = p[1]; p[1] = t; }
void swap32(u8* p) { u8 t; t=p[0]; p[0]=p[3]; p[3]=t; t=p[1]; p[1]=p[2]; p[2]=t; }
void swap64(u8* p) {
    u8 t;
    t=p[0]; p[0]=p[7]; p[7]=t;
    t=p[1]; p[1]=p[6]; p[6]=t;
    t=p[2]; p[2]=p[5]; p[5]=t;
    t=p[3]; p[3]=p[4]; p[4]=t;
}

void writeBE16(u8* p, u16 v) { p[0] = static_cast<u8>(v >> 8); p[1] = static_cast<u8>(v); }
void writeBE32(u8* p, u32 v) {
    p[0] = static_cast<u8>(v >> 24);
    p[1] = static_cast<u8>(v >> 16);
    p[2] = static_cast<u8>(v >> 8);
    p[3] = static_cast<u8>(v);
}

// Matches a 4-byte game code against the GC entries in REGIONS
// (REGIONS strings are 6+ chars, e.g. "GZ2E01" -> code "GZ2E", company "01").
bool isRecognizedGameCode(const u8* gameName, const u8* company) {
    for (size_t i = 0; i < NUM_REGIONS; i++) {
        const char* r = REGIONS[i];
        size_t len = std::strlen(r);
        if (len < 6) continue; // need at least 4-char code + 2-char company
        if (std::memcmp(r, gameName, 4) != 0) continue;
        if (r[4] != static_cast<char>(company[0])) continue;
        if (r[5] != static_cast<char>(company[1])) continue;
        return true;
    }
    return false;
}

// Returns the GC USA code (GZ2E) as the default for new-file creation.
constexpr u8 kDefaultGameName[4] = { 'G', 'Z', '2', 'E' };

void swapGCNSaveData(u8* base) {
    swap16(base + 0x000);
    swap16(base + 0x002);
    swap16(base + 0x004);
    swap16(base + 0x006);
    swap16(base + 0x008);

    swap64(base + 0x028);
    swap32(base + 0x034);
    swap16(base + 0x038);

    swap32(base + 0x040);
    swap32(base + 0x044);
    swap32(base + 0x048);
    swap16(base + 0x04C);

    swap32(base + 0x064);
    swap32(base + 0x068);
    swap32(base + 0x06C);
    swap16(base + 0x070);

    swap32(base + 0x080);
    swap32(base + 0x084);
    swap32(base + 0x088);
    swap16(base + 0x08C);

    for (int i = 0; i < 8; i++) swap32(base + 0x0CC + i * 4);

    swap32(base + 0x11C);
    swap32(base + 0x120);
    swap32(base + 0x124);
    swap32(base + 0x128);

    for (int i = 0; i < 16; i++) swap16(base + 0x16C + i * 2);

    swap64(base + 0x1A0);
    swap64(base + 0x1A8);
    swap16(base + 0x1B0);
    swap16(base + 0x1B2);

    swap16(base + 0x1E6);

    for (int m = 0; m < 32; m++) {
        u8* mem = base + 0x1F0 + m * 0x20;
        swap32(mem + 0x00);
        swap32(mem + 0x04);
        swap32(mem + 0x08);
        swap32(mem + 0x0C);
        swap32(mem + 0x10);
        swap32(mem + 0x14);
        swap32(mem + 0x18);
    }

    for (int m = 0; m < 64; m++) {
        u8* mem = base + 0x5F0 + m * 0x08;
        swap32(mem + 0x00);
        swap32(mem + 0x04);
    }

    swap32(base + 0x944);
    swap32(base + 0x948);
    swap32(base + 0x94C);
    swap32(base + 0x950);
    swap32(base + 0x954);
}

u64 gciCalcCheckSumGameData(u8* data, u32 size) {
    u32 high = 0, low = 0;
    for (u32 i = 0; i < size; i++) {
        high += data[i];
        low += ~data[i];
    }
    return (u64)high << 32 | low;
}

u32 gciCalcCheckSum(u8* data, u32 size) {
    u16 high = 0, low = 0;
    u16* d = (u16*)data;
    for (u32 i = 0; i < size / 2; i++) {
        high += d[i];
        low += (u16)~d[i];
    }
    return (u32)high << 16 | low;
}

void swapDataSectors(u8* cardData) {
    for (int sector = 0; sector < 2; sector++) {
        u8* ds = cardData + 0x4000 + sector * kDataSSize;

        swap32(ds + 0);
        swap32(ds + 4);

        u8* dataStart = ds + 8;
        for (int slot = 0; slot < 3; slot++) {
            u8* slotBase = dataStart + slot * kSavedataSize;

            swapGCNSaveData(slotBase);

            u64 checksum = gciCalcCheckSumGameData(slotBase, kSavedataSize - 8);
            u8* csPtr = slotBase + kSavedataSize - 8;
            memcpy(csPtr, &checksum, 8);
        }

        u32 outerCS = gciCalcCheckSum(ds, kDataSSize - 4);
        memcpy(ds + kDataSSize - 4, &outerCS, 4);
    }
}

class CRMemcardBackend final : public card::MemcardBackend {
protected:
    std::string resolveGciPath(int, std::string_view name) override {
        std::string fileName(name);
        if (fileName.empty() || fileName == "gczelda2") {
            fileName = Config::paths.active_save.value;
        }
        if (fileName.empty()) {
            fileName = "gczelda2";
        }

        const std::string folder = Paths::UserDirFolder(Config::paths.save_folder.value).string();
        if (fileName.size() >= 4 && fileName.compare(fileName.size() - 4, 4, kSaveExtension) == 0) {
            return folder + "/" + fileName;
        }
        return folder + "/" + fileName + kSaveExtension;
    }

    bool isRecognizedHeader(const u8 header[card::kGciHeaderSize]) override {
        return isRecognizedGameCode(header, header + 4);
    }

    void swapToNative(u8* data, size_t size) override {
        if (size < 0x4000 + 2 * kDataSSize) return;
        swapDataSectors(data);
    }

    void swapToDisk(u8* data, size_t size) override {
        if (size < 0x4000 + 2 * kDataSSize) return;
        swapDataSectors(data);
    }

    void prepareNewFile(card::File& file) override {
        std::memcpy(file.gameName, kDefaultGameName, sizeof(file.gameName));
        std::memcpy(file.company, kCompany, sizeof(file.company));

        file.bannerFormat = 0;
        file.iconAddr = 0xFFFFFFFF;
        file.commentAddr = 0xFFFFFFFF;
        file.iconFormat = 0;
        file.iconSpeed = 0;
    }

    void writeHeader(u8 header[card::kGciHeaderSize], const card::File& file) override {
        std::memset(header, 0, card::kGciHeaderSize);

        std::memcpy(header, file.gameName, 4);
        std::memcpy(header + 4, file.company, 2);

        header[0x07] = file.bannerFormat;

        std::string name = file.cardName;
        if (name.empty()) {
            const auto slash = file.gciPath.find_last_of("/\\");
            name = (slash == std::string::npos) ? file.gciPath
                                                : file.gciPath.substr(slash + 1);
            if (name.size() > 4 && name.compare(name.size() - 4, 4, ".gci") == 0)
                name.resize(name.size() - 4);
        }
        std::strncpy(reinterpret_cast<char*>(header + 0x08), name.c_str(), 32 - 1);

        writeBE32(header + 0x28, file.time);
        writeBE32(header + 0x2C, file.iconAddr);
        writeBE16(header + 0x30, file.iconFormat);
        writeBE16(header + 0x32, file.iconSpeed);

        header[0x34] = 0x04;
        header[0x35] = 0x00;
        writeBE16(header + 0x36, 0);

        const u16 blocks = static_cast<u16>((file.size + kSector - 1) / kSector);
        writeBE16(header + 0x38, blocks);

        writeBE32(header + 0x3C, file.commentAddr);
    }

    void ensureStorage(int) override {
        Paths::EnsureDirectory(Paths::UserDirFolder(Config::paths.save_folder.value));
    }

    void onFlushed(const card::File&) override {
        //pcSaveQuery_InvalidateCache();
    }
};

CRMemcardBackend s_backend;

} // namespace

void cr::rf::initCard()
{
    Paths::EnsureDirectory(
        Paths::UserDirFolder(Config::paths.save_folder.value));
    rainfall::card::SetBackend(&s_backend);
}
