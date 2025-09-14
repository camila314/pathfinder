cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(pathfinder VERSION 1.0.0)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/pathfinder.cpp
    src/checker.cpp
    src/debug.cpp
    # Add any extra C++ source files here
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
add_subdirectory(gd-sim)

CPMAddPackage("gh:camila314/UIBuilder#main")
CPMAddPackage("gh:maxnut/GDReplayFormat#gdr2")

# add custom target to strip binary
if (APPLE)
add_custom_target(strip ALL
    COMMAND $<$<CONFIG:Debug>:echo> strip -x $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
)
endif()


setup_geode_mod(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-DDEBUG_MODE=1>)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-g> -O3 -fno-math-errno -fno-trapping-math -fvisibility=hidden -fomit-frame-pointer)
target_link_libraries(${PROJECT_NAME} gd-sim UIBuilder libGDR)