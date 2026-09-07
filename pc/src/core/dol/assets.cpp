#include "cr/core/dol/assets.hpp"
#include "rainfall/platform/pc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>

#include "cr/utility/log.hpp"

CR_FILENAME_LOGGER();

namespace {

uint32_t be32(const unsigned char* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}

uint32_t host32(const unsigned char* p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

struct DolSection {
    uint32_t fileOff;
    uint32_t vaddr;
    uint32_t size;
};

struct DolImage {
    DolSection sec[18];
    int count;
    unsigned char* data;
    size_t dataSize;
};

DolImage  s_dol{};
bool      s_dolReady = false;
std::string s_gameDataDir;
std::unordered_map<std::string, std::pair<unsigned char*, size_t>> s_relCache;

bool LoadFile(const char* path, unsigned char** outBuf, size_t* outSize) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return false; }
    unsigned char* buf = (unsigned char*)malloc((size_t)sz);
    if (!buf) { fclose(f); return false; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return false;
    }
    fclose(f);
    *outBuf = buf;
    *outSize = (size_t)sz;
    return true;
}

bool ParseDol(unsigned char* buf, size_t bufSize, DolImage* img) {
    if (bufSize < 0x100) return false;
    img->data = buf;
    img->dataSize = bufSize;
    img->count = 0;
    for (int i = 0; i < 7; i++) {
        uint32_t off  = be32(buf + 0x00 + i*4);
        uint32_t addr = be32(buf + 0x48 + i*4);
        uint32_t size = be32(buf + 0x90 + i*4);
        if (off && size) {
            img->sec[img->count++] = { off, addr, size };
        }
    }
    for (int i = 0; i < 11; i++) {
        uint32_t off  = be32(buf + 0x1C + i*4);
        uint32_t addr = be32(buf + 0x48 + 28 + i*4);
        uint32_t size = be32(buf + 0x90 + 28 + i*4);
        if (off && size) {
            img->sec[img->count++] = { off, addr, size };
        }
    }
    return true;
}

bool TryRelPath(const std::string& base, const char* relName, std::string* outPath) {
    std::string p = base + "/" + relName + ".rel";
    FILE* f = fopen(p.c_str(), "rb");
    if (f) { fclose(f); *outPath = std::move(p); return true; }
    return false;
}

unsigned char* DecompressYaz0(const unsigned char* src, size_t srcSize, size_t* outSize) {
    if (srcSize < 16 || memcmp(src, "Yaz0", 4) != 0) return nullptr;
    uint32_t decompSize = be32(src + 4);
    if (decompSize > 64u * 1024u * 1024u) return nullptr;
    unsigned char* dst = (unsigned char*)malloc(decompSize);
    if (!dst) return nullptr;
    const unsigned char* srcEnd = src + srcSize;
    const unsigned char* srcPtr = src + 16;
    unsigned char* dstPtr = dst;
    unsigned char* dstEnd = dst + decompSize;
    while (dstPtr < dstEnd && srcPtr < srcEnd) {
        uint8_t code = *srcPtr++;
        for (int i = 0; i < 8 && dstPtr < dstEnd; i++) {
            if (code & 0x80) {
                if (srcPtr >= srcEnd) break;
                *dstPtr++ = *srcPtr++;
            } else {
                if (srcPtr + 2 > srcEnd) break;
                uint8_t b1 = *srcPtr++;
                uint8_t b2 = *srcPtr++;
                uint32_t dist = ((b1 & 0x0F) << 8) | b2;
                uint32_t count;
                if ((b1 >> 4) == 0) {
                    if (srcPtr >= srcEnd) break;
                    count = *srcPtr++ + 0x12;
                } else {
                    count = (b1 >> 4) + 2;
                }
                if (dist + 1 > (uint32_t)(dstPtr - dst)) { free(dst); return nullptr; }
                unsigned char* backPtr = dstPtr - dist - 1;
                for (uint32_t j = 0; j < count && dstPtr < dstEnd; j++) {
                    *dstPtr++ = *backPtr++;
                }
            }
            code <<= 1;
        }
    }
    *outSize = decompSize;
    return dst;
}

bool ExtractFromRARC(const unsigned char* arc, size_t arcSize, const char* fileName,
                     unsigned char** outBuf, size_t* outSize) {
    if (arcSize < 0x60) return false;
    if (host32(arc) != 0x52415243u) return false;

    uint32_t headerLen     = host32(arc + 0x08);
    uint32_t fileDataOff   = host32(arc + 0x0C);
    if ((uint64_t)headerLen + 0x20 > arcSize) return false;

    const unsigned char* info = arc + headerLen;
    uint32_t numFiles      = host32(info + 0x08);
    uint32_t fileEntryOff  = host32(info + 0x0C);
    uint32_t stringTableOff= host32(info + 0x14);

    const unsigned char* files   = info + fileEntryOff;
    const char*          strings = (const char*)info + stringTableOff;
    const unsigned char* dataBase= arc + headerLen + fileDataOff;

    if ((uint64_t)(files - arc) + (uint64_t)numFiles * 20 > arcSize) return false;

    for (uint32_t i = 0; i < numFiles; i++) {
        const unsigned char* e = files + i * 20;
        uint32_t typeFlagsName = host32(e + 0x04);
        uint32_t flags         = (typeFlagsName >> 24) & 0xFF;
        uint32_t nameOff       = typeFlagsName & 0xFFFFFF;
        if (flags & 0x02) continue;
        const char* name = strings + nameOff;
        if (strcmp(name, fileName) != 0) continue;

        uint32_t dataOff  = host32(e + 0x08);
        uint32_t dataSize = host32(e + 0x0C);
        if ((uint64_t)(dataBase - arc) + dataOff + dataSize > arcSize) return false;

        unsigned char* buf = (unsigned char*)malloc(dataSize);
        if (!buf) return false;
        memcpy(buf, dataBase + dataOff, dataSize);
        *outBuf = buf;
        *outSize = dataSize;
        return true;
    }
    return false;
}

unsigned char* s_relsArcData = nullptr;
size_t         s_relsArcSize = 0;
bool           s_relsArcTried = false;

bool LoadRELSArc() {
    if (s_relsArcData) return true;
    if (s_relsArcTried) return false;
    s_relsArcTried = true;
    if (s_gameDataDir.empty()) return false;
    std::string path = s_gameDataDir + "/files/RELS.arc";
    if (!LoadFile(path.c_str(), &s_relsArcData, &s_relsArcSize)) {
        LOG_ERROR("RELS.arc not found at {}", path);
        return false;
    }
    LOG_INFO("RELS.arc loaded from {} ({} bytes)", path, s_relsArcSize);
    return true;
}

bool LoadRelByName(const char* relName, unsigned char** outBuf, size_t* outSize) {
    auto it = s_relCache.find(relName);
    if (it != s_relCache.end()) {
        *outBuf = it->second.first;
        *outSize = it->second.second;
        return true;
    }
    if (s_gameDataDir.empty()) {
        LOG_ERROR("rel {} requested but no game data dir set", relName);
        return false;
    }

    static const char* kCandidates[] = {
        "files/rel/Rfinal/Release",
        "files/rel/Final/Release",
        "files/RELS/rels/amem",
        "files/RELS/rels/mmem",
    };
    std::string path;
    for (auto sub : kCandidates) {
        std::string base = s_gameDataDir + "/" + sub;
        if (TryRelPath(base, relName, &path)) break;
    }
    if (!path.empty()) {
        unsigned char* buf = nullptr;
        size_t sz = 0;
        if (!LoadFile(path.c_str(), &buf, &sz)) {
            LOG_ERROR("rel {} found at {} but read failed", relName, path);
            return false;
        }
        if (sz >= 4 && memcmp(buf, "Yaz0", 4) == 0) {
            size_t decSize = 0;
            unsigned char* dec = DecompressYaz0(buf, sz, &decSize);
            free(buf);
            if (!dec) {
                LOG_ERROR("rel {} Yaz0 decompress failed", relName);
                return false;
            }
            buf = dec;
            sz = decSize;
            LOG_INFO("rel {} loaded from {} (Yaz0-decompressed, {} bytes)", relName, path, sz);
        } else {
            LOG_INFO("rel {} loaded from {} ({} bytes)", relName, path, sz);
        }
        s_relCache[relName] = { buf, sz };
        *outBuf = buf;
        *outSize = sz;
        return true;
    }

    if (LoadRELSArc()) {
        std::string entryName = std::string(relName) + ".rel";
        unsigned char* buf = nullptr;
        size_t sz = 0;
        if (ExtractFromRARC(s_relsArcData, s_relsArcSize, entryName.c_str(), &buf, &sz)) {
            if (sz >= 4 && memcmp(buf, "Yaz0", 4) == 0) {
                size_t decSize = 0;
                unsigned char* dec = DecompressYaz0(buf, sz, &decSize);
                free(buf);
                if (!dec) {
                    LOG_ERROR("rel {} Yaz0 decompress failed", relName);
                    return false;
                }
                buf = dec;
                sz = decSize;
                LOG_INFO("rel {} extracted+Yaz0-decompressed from RELS.arc ({} bytes)", relName, sz);
            } else {
            LOG_INFO("rel {} extracted from RELS.arc ({} bytes)", relName, sz);
            }
            s_relCache[relName] = { buf, sz };
            *outBuf = buf;
            *outSize = sz;
            return true;
        }
        LOG_ERROR("rel {} not in RELS.arc either", relName);
    }

    LOG_ERROR("rel {} not found under {} (tried rel/Rfinal/Release, rel/Final/Release, "
             "RELS/rels/{{amem,mmem}}, RELS.arc)",
             relName, s_gameDataDir);
    return false;
}

} // namespace

namespace cr::dol {

bool init(const char* gameDataDir) {
    LOG_INFO("init: enter dir={}", gameDataDir ? gameDataDir : "(null)");
    if (!gameDataDir || !gameDataDir[0]) {
        LOG_ERROR("init: no game data dir");
        return false;
    }
    if (s_dolReady && s_gameDataDir == gameDataDir) {
        return true;
    }
    if (s_dol.data) {
        free(s_dol.data);
        s_dol = {};
        s_dolReady = false;
    }
    for (auto& kv : s_relCache) {
        free(kv.second.first);
    }
    s_relCache.clear();
    s_gameDataDir = gameDataDir;

    std::string dolPath = s_gameDataDir + "/sys/main.dol";
    unsigned char* buf = nullptr;
    size_t sz = 0;
    if (!LoadFile(dolPath.c_str(), &buf, &sz)) {
        LOG_ERROR("init: failed to open {}", dolPath);
        return false;
    }
    if (!ParseDol(buf, sz, &s_dol)) {
        LOG_ERROR("init: main.dol parse failed");
        free(buf);
        return false;
    }
    s_dolReady = true;
    LOG_INFO("init: gameDataDir={} main.dol={} bytes, {} sections", s_gameDataDir, sz, s_dol.count);

    static const char* kKnownRels[] = { "d_a_mant", "d_a_grass" };
    for (const char* relName : kKnownRels) {
        unsigned char* tmp = nullptr; size_t tmpSz = 0;
        if (!LoadRelByName(relName, &tmp, &tmpSz)) {
            LOG_WARN("init: {}.rel not found at startup", relName);
        }
    }
    return true;
}

void patchMatDLSetImage3(void* matDL, uint32_t matDLSize, const void* texPtr) {
    if (!matDL || !texPtr || matDLSize < 5) return;
    uint32_t addr = (uint32_t)((uintptr_t)texPtr >> 5);
    unsigned char* dl = (unsigned char*)matDL;
    uint32_t scanLimit = matDLSize < 0x40 ? matDLSize : 0x40;
    for (uint32_t i = 0; i + 5 <= scanLimit; i++) {
        if (dl[i] == 0x61 && dl[i+1] == 0x94) {
            dl[i+2] = (unsigned char)((addr >> 16) & 0xFF);
            dl[i+3] = (unsigned char)((addr >> 8)  & 0xFF);
            dl[i+4] = (unsigned char)( addr        & 0xFF);
            return;
        }
    }
    LOG_WARN("matDL has no SETIMAGE3_TEX0 in first 0x{:X} bytes; texture won't bind via DL", scanLimit);
}

void patchMatDLTex0Dims(void* matDL, uint32_t matDLSize, uint16_t width, uint16_t height, uint8_t format) {
    if (!matDL || matDLSize < 5 || width == 0 || height == 0) return;
    uint32_t bpData = ((uint32_t)(format & 0x0F) << 20)
                    | (((uint32_t)(height - 1) & 0x3FF) << 10)
                    |  ((uint32_t)(width - 1)  & 0x3FF);
    unsigned char* dl = (unsigned char*)matDL;
    uint32_t scanLimit = matDLSize < 0x40 ? matDLSize : 0x40;
    for (uint32_t i = 0; i + 5 <= scanLimit; i++) {
        if (dl[i] == 0x61 && dl[i+1] == 0x88) {
            dl[i+2] = (unsigned char)((bpData >> 16) & 0xFF);
            dl[i+3] = (unsigned char)((bpData >> 8)  & 0xFF);
            dl[i+4] = (unsigned char)( bpData        & 0xFF);
            return;
        }
    }
    LOG_WARN("matDL has no TEX0_DIMS (BP 0x88) in first 0x{:X} bytes", scanLimit);
}

bool loadDolRange(void* dst, uint32_t vaddr, uint32_t size) {
    if (!s_dolReady) return false;
    for (int i = 0; i < s_dol.count; i++) {
        const DolSection& s = s_dol.sec[i];
        if (vaddr >= s.vaddr && (uint64_t)vaddr + size <= (uint64_t)s.vaddr + s.size) {
            uint32_t off = s.fileOff + (vaddr - s.vaddr);
            if ((uint64_t)off + size > s_dol.dataSize) return false;
            memcpy(dst, s_dol.data + off, size);
            pcEmbeddedBlobRegister(dst, size);
            return true;
        }
    }
    LOG_ERROR("dol range not found vaddr=0x{:08X} size=0x{:X}", vaddr, size);
    return false;
}

bool loadRelRange(void* dst, const char* relName, uint32_t fileOffset, uint32_t size) {
    unsigned char* buf = nullptr;
    size_t sz = 0;
    if (!LoadRelByName(relName, &buf, &sz)) {
        LOG_ERROR("rel not found: {}", relName);
        return false;
    }
    if ((uint64_t)fileOffset + size > sz) {
        LOG_ERROR("rel range OOB {} off=0x{:X} size=0x{:X} relSz={}", relName, fileOffset, size, sz);
        return false;
    }
    memcpy(dst, buf + fileOffset, size);
    pcEmbeddedBlobRegister(dst, size);
    return true;
}

}
