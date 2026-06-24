cmake_minimum_required (VERSION 3.31)

project(DragonslayerProject)

# ----------------------------------------
# C++ standard
# ----------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------
# Platform Setup
# ----------------------------------------

# OS
set(DS_LINUX FALSE)
set(DS_WINDOWS FALSE)
if(WIN32)
    set(DS_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(DS_LINUX TRUE)
else()
    message(FATAL_ERROR "Unsupported operative system!")
endif()

# Compiler
set(DS_CLANG FALSE)
set(DS_GCC FALSE)
set(DS_MSVC FALSE)

set(DS_MSVC_COMPILER_FRONTEND FALSE)
set(DS_GNU_COMPILER_FRONTEND FALSE)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(DS_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(DS_GCC TRUE)
elseif(MSVC)
    set(DS_MSVC TRUE)
else()
    message(FATAL_ERROR "Unsupported compiler!")
endif()

if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(DS_MSVC_COMPILER_FRONTEND TRUE)
elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    set(DS_GNU_COMPILER_FRONTEND TRUE)
else()
    if(MSVC)
        set(DS_MSVC_COMPILER_FRONTEND TRUE)
    else()
        set(DS_GNU_COMPILER_FRONTEND TRUE)
    endif()
endif()

# ----------------------------------------
# Build directory
# ----------------------------------------

# Define output directory for builds
set(BUILD_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Build/$<CONFIG>/${CMAKE_SYSTEM_NAME}/${CMAKE_CXX_COMPILER_ID}")

# Apply it to runtimes (executables) and libraries (shared libs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_OUTPUT_DIR}")

# ----------------------------------------
# Hot-reload setup
# ----------------------------------------

option(DS_ENABLE_HOT_RELOADING "Enables Hot-reloading for Dragonslayer Engine" ON)

if(DS_ENABLE_HOT_RELOADING)
    message(STATUS "Hot-reloading enabled")
else()
    message(STATUS "Hot-reloading disabled")
endif()

# Moves the old PDB out of the way before linking so the debugger's lock
# on it does not block the linker during hot-reload builds.
# Windows allows renaming files that are open by another process.
macro(movePDB target)
    add_custom_command(TARGET ${target} PRE_LINK
        COMMAND ${CMAKE_COMMAND}
        -DPDB_PATH=$<TARGET_PDB_FILE:${target}>
        -P ${CMAKE_SOURCE_DIR}/MovePDB.cmake
        COMMENT "Moving old PDB out of the way for hot-reload"
    )
endmacro()

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:debug>:Debug>DLL")
# else()
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
# endif()

# TODO Disabling exceptions
# add_compile_options(
#     $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-exceptions>
#     $<$<CXX_COMPILER_ID:MSVC>:/EHs->
# )

# Include sub-projects.
add_subdirectory ("DragonslayerEngine")
add_subdirectory ("Game")

