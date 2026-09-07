# =============================================================================
# PC Port Shims / Code
# =============================================================================

file(GLOB_RECURSE PORT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
