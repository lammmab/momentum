#ifndef GALE01_40CD40
#define GALE01_40CD40

#include <Runtime/platform.h>

typedef struct TextGlyphTexture {
    /*0x00*/ u8 data[512];
} TextGlyphTexture;

/* 40CD40 */ extern TextGlyphTexture HSD_SisLib_FontAtlas[287];

#ifdef PLATFORM_PC
#define HSD_SISLIB_FONT_GLYPHS 287
#define HSD_SISLIB_FONT_SIZEOF (HSD_SISLIB_FONT_GLYPHS * sizeof(TextGlyphTexture))
#endif

#endif
