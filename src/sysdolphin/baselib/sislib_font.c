#include "sislib_font.h"

#ifndef PLATFORM_PC
TextGlyphTexture HSD_SisLib_FontAtlas[] ATTRIBUTE_ALIGN(32) = {
#include <sysdolphin/baselib/sislib_font.inc>
};
#endif
