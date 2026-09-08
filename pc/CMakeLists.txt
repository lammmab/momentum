cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Enable compile_commands.json generation" FORCE)

project(momentumReborn VERSION 0.1.0 LANGUAGES C CXX)

if(CMAKE_SYSTEM_NAME MATCHES "^(Darwin|iOS)$" AND NOT CMAKE_HOST_APPLE)
    set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-rpath,")
    set(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG "-Wl,-rpath,")
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# =============================================================================
# Options
# =============================================================================

option(USE_CPM "Fallback to CPM if a dependency isn't found" ON)
option(USE_TRACY "Enable Tracy profiler (requires having Tracy installed)" OFF)
option(GAME_DEBUG "Enable the original game's debug functionality" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# =============================================================================
# Sanitizers (Undefined Behavior & Addresses)
# =============================================================================

if(ENABLE_ASAN OR ENABLE_UBSAN)
    add_library(sanitizers INTERFACE)

    if(ENABLE_ASAN)
        target_compile_options(sanitizers INTERFACE
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        target_link_options(sanitizers INTERFACE
            -fsanitize=address
        )
    endif()

    if(ENABLE_UBSAN)
        target_compile_options(sanitizers INTERFACE
            -fsanitize=undefined
        )
        target_link_options(sanitizers INTERFACE
            -fsanitize=undefined
        )
    endif()
endif()

# =============================================================================
# Source Files
# =============================================================================

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/game.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/port.cmake)

# =============================================================================
# Target - Executable & App Bundle
# =============================================================================

set(APP_NAME "Momentum" CACHE STRING "Application display name")
set(APP_VERSION "${PROJECT_VERSION}" CACHE STRING "Application version string")
set(MAC_BUNDLE_ID "org.linifadomra.momentum" CACHE STRING
    "macOS bundle identifier for the app bundle (must match SDL_SetAppMetadata)")

if(APPLE)
    add_executable(momentum MACOSX_BUNDLE ${GAME_SOURCES} ${PORT_SOURCES})
    set_target_properties(momentum PROPERTIES
        OUTPUT_NAME "Momentum"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${MAC_BUNDLE_ID}"
        MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
        MACOSX_BUNDLE_EXECUTABLE_NAME "Momentum"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MAC_BUNDLE_ID}"
    )
else()
    add_executable(momentum ${GAME_SOURCES} ${PORT_SOURCES})
    set_target_properties(momentum PROPERTIES OUTPUT_NAME "Momentum")
endif()

# =============================================================================
# Setup: Architecture & compile definitions
# =============================================================================

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/setup/compile_defs.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/setup/arch.cmake)

# =============================================================================
# Dependencies
# =============================================================================

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps/dependencies.cmake)

# =============================================================================
# Include Directories
# =============================================================================

target_include_directories(momentum PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${GAME_ROOT}/include"
    "${GAME_ROOT}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/rainfall/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/rainfall/external/stb"
)

if(MSVC)
    target_compile_options(momentum PRIVATE
        /permissive
        /wd4244 /wd4267 /wd4305 /wd4996 /wd5033 /wd4005 /wd4838
        /we4302 /we4311 /we4312
        /bigobj
        $<$<CONFIG:RelWithDebInfo>:/Oy->
        $<$<CONFIG:RelWithDebInfo>:/Zo>)
    target_compile_definitions(momentum PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_options(momentum PRIVATE
        /INCREMENTAL:NO
        $<$<CONFIG:Debug>:/DEBUG:FULL>
        $<$<CONFIG:RelWithDebInfo>:/DEBUG:FULL>)
endif()

# =============================================================================
# Helper Libraries & Linking
# =============================================================================

if(WIN32 AND MSVC)
    target_link_libraries(momentum PRIVATE rainfall)
    target_link_options(momentum PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE_NAME:rainfall>")
elseif(WIN32)
    target_link_libraries(momentum PRIVATE
        -Wl,--whole-archive
        rainfall
        -Wl,--no-whole-archive)
elseif(APPLE)
    target_link_libraries(momentum PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,rainfall>")
else()
    target_link_libraries(momentum PRIVATE rainfall)
endif()

target_link_libraries(momentum PRIVATE
    siphon
    rainfall
    spdlog
    gcesfa
    $<$<TARGET_EXISTS:sanitizers>:sanitizers>
)

if(APPLE)
    target_link_libraries(momentum PRIVATE "-framework CoreFoundation")
endif()

if(WIN32)
    target_link_libraries(momentum PRIVATE dbghelp)
endif()


# =============================================================================
# Versioning
# =============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/momentum/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/momentum_version.hpp
)

# =============================================================================
# Post-Build Assets & Dynamic Libraries Copying
# =============================================================================

foreach(_runtime_dep dxcompiler dxildll)
    if(TARGET ${_runtime_dep})
        add_custom_command(TARGET momentum POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${_runtime_dep}>" "$<TARGET_FILE_DIR:momentum>"
            VERBATIM)
    endif()
endforeach()

# The Windows SDK ships a dxcompiler.dll without a SPIR-V backend, which SDL_shadercross
# needs for Vulkan. Prefer an override DLL when one is available.
if(WIN32 AND NOT CR_DXC_RUNTIME_DLL)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_cr_dxc_arch_dir "x64")
    else()
        set(_cr_dxc_arch_dir "x86")
    endif()
    set(_cr_dxc_bundled "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dxc/${_cr_dxc_arch_dir}/dxcompiler.dll")
    if(EXISTS "${_cr_dxc_bundled}")
        set(CR_DXC_RUNTIME_DLL "${_cr_dxc_bundled}" CACHE FILEPATH
            "dxcompiler.dll deployed next to the executable (must support SPIR-V)")
    elseif(DEFINED ENV{VULKAN_SDK} AND EXISTS "$ENV{VULKAN_SDK}/Bin/dxcompiler.dll")
        set(CR_DXC_RUNTIME_DLL "$ENV{VULKAN_SDK}/Bin/dxcompiler.dll" CACHE FILEPATH
            "dxcompiler.dll deployed next to the executable (must support SPIR-V)")
    else()
        message(WARNING
            "No SPIR-V capable dxcompiler.dll found - the Vulkan backend will fail to "
            "compile shaders at runtime (OpenGL is unaffected; run with --backend opengl).\n"
            "Fix by any one of:\n"
            "  - install the Vulkan SDK (provides Bin/dxcompiler.dll)\n"
            "  - drop a SPIR-V capable dxcompiler.dll in "
            "pc/thirdparty/dxc/${_cr_dxc_arch_dir}/ (from the DirectXShaderCompiler releases)\n"
            "  - configure with -DCR_DXC_RUNTIME_DLL=<path to dxcompiler.dll>")
    endif()
endif()

if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    add_custom_command(TARGET momentum POST_BUILD
        COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:momentum>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:momentum>;$<TARGET_FILE_DIR:momentum>>"
        COMMAND_EXPAND_LISTS)
endif()

if(NOT APPLE AND TARGET DirectXShaderCompiler::dxcompiler)
    foreach(_dxc_imported DirectXShaderCompiler::dxcompiler DirectXShaderCompiler::dxil)
        if(TARGET ${_dxc_imported})
            get_target_property(_dxc_imported_loc ${_dxc_imported} IMPORTED_LOCATION)
            if(_dxc_imported_loc AND _dxc_imported_loc MATCHES "\.(dll|so|dylib)$")
                add_custom_command(TARGET momentum POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_dxc_imported_loc}" "$<TARGET_FILE_DIR:momentum>"
                    VERBATIM)
            endif()
        endif()
    endforeach()
endif()

# Runs last so it wins over any dxcompiler.dll copied by the rules above.
if(WIN32 AND CR_DXC_RUNTIME_DLL)
    add_custom_command(TARGET momentum POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CR_DXC_RUNTIME_DLL}" "$<TARGET_FILE_DIR:momentum>"
        VERBATIM)
endif()

if(APPLE AND TARGET DirectXShaderCompiler::dxcompiler)
    get_target_property(_dxc_loc DirectXShaderCompiler::dxcompiler IMPORTED_LOCATION)
    if(_dxc_loc)
        get_filename_component(_dxc_dir "${_dxc_loc}" DIRECTORY)
        set_property(TARGET momentum APPEND PROPERTY BUILD_RPATH "${_dxc_dir}")
        add_custom_command(TARGET momentum POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_dxc_loc}" "$<TARGET_FILE_DIR:momentum>"
            VERBATIM)
    endif()
endif()
