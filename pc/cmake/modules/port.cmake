# =============================================================================
# PC Port Shims / Code
# =============================================================================

file(GLOB_RECURSE PORT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

list(FILTER PORT_SOURCES EXCLUDE REGEX "/platform_windows\\.cpp$")
list(FILTER PORT_SOURCES EXCLUDE REGEX "/platform_posix\\.cpp$")

if(WIN32)
    list(APPEND PORT_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/impl/platform_windows.cpp")
else()
    list(APPEND PORT_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/impl/platform_posix.cpp")
endif()