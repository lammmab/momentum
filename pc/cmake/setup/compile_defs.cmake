# =============================================================================
# Preprocessor Definitions & App Metadata
# =============================================================================

target_compile_definitions(momentum PRIVATE
    # ppds
    PLATFORM_PC=1
    PLATFORM_GCN=0
    PLATFORM_WII=0
    PLATFORM_SHIELD=0
    WIDESCREEN_SUPPORT=1

    # version
    VERSION=VERSION_GCN_USA
    VERSION_GCN_USA=0
    VERSION_GCN_PAL=1
    VERSION_GCN_JPN=2

    # metadata
    "APP_NAME=\"${APP_NAME}\""
    "APP_VERSION=\"${APP_VERSION}\""
    "MAC_BUNDLE_ID=\"${MAC_BUNDLE_ID}\""
)
