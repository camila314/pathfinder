cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(gd-sim VERSION 0.1.0 LANGUAGES CXX)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/**/*.cpp)
add_library(gd-sim STATIC ${SOURCES})

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(gd-sim PRIVATE $<$<CONFIG:Debug>:-g> -O3 -fno-math-errno -fno-trapping-math -fomit-frame-pointer $<$<CONFIG:Release>:-fno-rtti -fvisibility=hidden>)


target_include_directories(gd-sim PUBLIC include)

target_compile_definitions(gd-sim PRIVATE -D_HAS_ITERATOR_DEBUGGING=0)

add_executable(gd-sim-test ${SOURCES} test/test.cpp)

set_target_properties(gd-sim-test PROPERTIES INTERPROCEDURAL_OPTIMIZATION 0)
target_include_directories(gd-sim-test PRIVATE include)