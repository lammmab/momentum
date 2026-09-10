#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

enum WindowMode {
    WINDOWED            = 0,
    WINDOWED_BORDERLESS = 1,
    FULLSCREEN          = 2
};

enum DisplayMode {
    STANDARD   = 0,
    WIDESCREEN = 1,
    ULTRAWIDE  = 2
};

enum BackendChoice {
    AUTO     = 0,
    SDL3_GPU = 1,
    OPENGL   = 2
};

enum CreactMode {
    CREACT_FULL       = 0,
    CREACT_PAUSE_ONLY = 1,
    CREACT_DISABLED   = 2
};

#endif
