# =============================================================================
# Architecture & Compiler Configuration
# =============================================================================

if(NOT MSVC)
    add_compile_options(-Wno-everything)
endif()

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

if(MSVC OR APPLE OR UNIX)
    option(PC_32BIT "Build 32-bit" OFF)
else()
    option(PC_32BIT "Build 32-bit" ON)
endif()

if(PC_32BIT AND NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_BITS 64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH_BITS 32)
else()
    message(FATAL_ERROR "Unsupported pointer size: ${CMAKE_SIZEOF_VOID_P}")
endif()

message(STATUS "Target architecture: ${ARCH_BITS}-bit")
