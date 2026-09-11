# =============================================================================
# Original Game Source Files
# =============================================================================

set(GAME_SRC_DIRS
    melee
    sysdolphin
    jmp
)

set(GAME_SOURCES ${GAME_ROOT}/src/melee/gm/gmmain.c)
foreach(dir ${GAME_SRC_DIRS})
    file(GLOB_RECURSE _dir_sources "${GAME_ROOT}/src/${dir}/*.cpp" "${GAME_ROOT}/src/${dir}/*.c")
    list(APPEND GAME_SOURCES ${_dir_sources})
endforeach()

# Excludes a source file from GAME_SOURCES by exact filename (basename match)
# Usage: exclude_game_source(<filename> "<reason>")
function(exclude_game_source filename reason)
    set(_matched FALSE)
    foreach(src ${GAME_SOURCES})
        get_filename_component(_basename ${src} NAME)
        if(_basename STREQUAL filename)
            list(REMOVE_ITEM GAME_SOURCES ${src})
            set(_matched TRUE)
        endif()
    endforeach()
    if(NOT _matched)
        message(WARNING "exclude_game_source: '${filename}' not found in GAME_SOURCES (reason: ${reason})")
    endif()
    set(GAME_SOURCES ${GAME_SOURCES} PARENT_SCOPE)
endfunction()

# Excludes all source files under a directory path fragment
# Usage: exclude_game_source_dir(<path_fragment> "<reason>")
function(exclude_game_source_dir path_fragment reason)
    set(_kept "")
    set(_matched FALSE)
    foreach(src ${GAME_SOURCES})
        if(src MATCHES "${path_fragment}")
            set(_matched TRUE)
        else()
            list(APPEND _kept ${src})
        endif()
    endforeach()
    if(NOT _matched)
        message(WARNING "exclude_game_source_dir: '${path_fragment}' matched nothing (reason: ${reason})")
    endif()
    set(GAME_SOURCES ${_kept} PARENT_SCOPE)
endfunction()

#exclude_game_source(d_a_movie_player.cpp "PC replacement used instead")
