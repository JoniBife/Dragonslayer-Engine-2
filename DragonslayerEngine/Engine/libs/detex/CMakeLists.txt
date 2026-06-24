cmake_minimum_required(VERSION 3.31)

project(Detex)

set(COMPILE_DEFINITIONS "")
if(WIN32)
    set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB DETEX_SOURCE "*.c" "*.h")
source_group("" FILES ${DETEX_SOURCE})

add_library(Detex STATIC ${DETEX_SOURCE})

target_compile_definitions(Detex PRIVATE ${COMPILE_DEFINITIONS})

target_include_directories(Detex PUBLIC "")