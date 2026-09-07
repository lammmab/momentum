set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm-cache" CACHE PATH "CPM source cache")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps/CPM.cmake")

if(NOT COMMAND CPMAddPackage)
    message(FATAL_ERROR
        "CPM.cmake was included, but CPMAddPackage was not defined. "
        "Check cmake/deps/CPM.cmake."
    )
endif()

function(grab_dep pkg_name repo tag use_config)
    if(use_config)
        find_package(${pkg_name} CONFIG QUIET)
    else()
        find_package(${pkg_name} QUIET)
    endif()
    if(NOT ${pkg_name}_FOUND)
        message(STATUS "${pkg_name} was not found via find_package() and we must grab it locally (via CPM).")
        CPMAddPackage(
            NAME ${pkg_name}
            GITHUB_REPOSITORY ${repo}
            GIT_TAG ${tag}
        )
    endif()
endfunction()

set(CONFLUENCE_DOL_YAML OFF CACHE BOOL "" FORCE)
CPMAddPackage(
    NAME Confluence
    GIT_REPOSITORY https://github.com/Linifadomra/Confluence
    GIT_TAG a7f6fc07ceb76b88f9297dcbcc79c8b9c475418b
)

if(TARGET confluence AND NOT TARGET Confluence::confluence)
    add_library(Confluence::confluence ALIAS confluence)
endif()

set(Confluence_FOUND TRUE CACHE BOOL "" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

unset(ZLIB_CONF_WRITTEN CACHE)
if(NOT TARGET ZLIB::ZLIB)
    grab_dep(
        ZLIB
        madler/zlib
        v1.3.2
        FALSE
    )
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/siphon")

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    find_package(SDL3 CONFIG QUIET)
endif()

if(APPLE)
    if(EXISTS "/opt/homebrew/lib/cmake/SDL3/SDL3Config.cmake")
        set(SDL3_DIR "/opt/homebrew/lib/cmake/SDL3" CACHE PATH "Homebrew SDL3" FORCE)
        find_package(SDL3 CONFIG QUIET)
    endif()
endif()

if(NOT SDL3_FOUND)
    message(STATUS "SDL3 not found via find_package; fetching it locally (via CPM).")
    CPMAddPackage(
        NAME SDL3
        GITHUB_REPOSITORY libsdl-org/SDL
        GIT_TAG release-3.4.8
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(RAINFALL_SHADERCROSS_CLI OFF CACHE BOOL "" FORCE)
else()
    set(RAINFALL_SHADERCROSS_CLI ON CACHE BOOL "" FORCE)
endif()

set(RAINFALL_POST_PROCESSING OFF CACHE BOOL "" FORCE)

set(RAINFALL_INCLUDE_AUDIO ON CACHE BOOL "" FORCE)
set(RAINFALL_INCLUDE_MOVIE ON CACHE BOOL "" FORCE)
set(RAINFALL_HOST_USES_SSYSTEM ON CACHE BOOL "" FORCE)
set(RAINFALL_HOST_USES_JSYSTEM ON CACHE BOOL "" FORCE)

set(RAINFALL_USE_INICPP ON CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/rainfall")

target_include_directories(rainfall PUBLIC
    "${GAME_ROOT}/libs/JSystem/include"
    "${GAME_ROOT}/include"
)

target_compile_definitions(rainfall PUBLIC 
    PLATFORM_PC=1 
    $<$<BOOL:${GAME_DEBUG}>:DEBUG=1>
)

if(USE_TRACY)
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    CPMAddPackage("gh:wolfpld/tracy#1690ac0d9d262b4bcfa6006824bedbf367f6cb0b")
endif()

if(NOT BUILD_SWITCH)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(_courage_saved_bsl "${BUILD_SHARED_LIBS}")
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    endif()
    grab_dep(spdlog gabime/spdlog v1.17.0 TRUE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(BUILD_SHARED_LIBS "${_courage_saved_bsl}" CACHE BOOL "" FORCE)
        unset(_courage_saved_bsl)
    endif()
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/gcesfa")
