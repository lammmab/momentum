#pragma once

#ifndef REGION_FORMAT_H
#define REGION_FORMAT_H

#include <cstdint>
#include <array>

// Disc image format magic bytes (file byte order, from Dolphin source).
constexpr std::array<std::array<uint8_t,4>,7> IMAGE_FORMATS = {{
    {'C','I','S','O'},      // CISO
    {'W','I','A',0x01},     // WIA
    {'R','V','Z',0x01},     // RVZ
    {0x01,0xC0,0x0B,0xB1},  // GCZ
    {0xAE,0x0F,0x38,0xA2},  // TGC
    {'W','B','F','S'},      // WBFS
    {'E','G','G','S'}       // NFS
}};

// Raw discs carry no container magic. ISO and GCM
constexpr std::array<std::array<uint8_t,6>,6> RAW_DISC_IDS = {{
    {'G','A','L','E','0','1'}
}};

constexpr const char* REGIONS[] = {
    "GALE01",
};
constexpr size_t NUM_REGIONS = sizeof(REGIONS) / sizeof(REGIONS[0]);

#endif
