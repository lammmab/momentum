#include "cr/platform/common/sdl.hpp"

#include <SDL3/SDL.h>

namespace cr::platform::common {
    void sdl::InitializeSDLMeta() {
        SDL_SetAppMetadata(APP_NAME, APP_VERSION, nullptr);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, APP_NAME);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, MAC_BUNDLE_ID);
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, "https://github.com/Linifadomra");
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
    }
}