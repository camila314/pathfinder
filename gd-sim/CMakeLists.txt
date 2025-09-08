cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)
project(gd-sim VERSION 0.1.0 LANGUAGES CXX)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/**/*.cpp)
add_library(gd-sim STATIC ${SOURCES})

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(gd-sim PRIVATE $<$<CONFIG:Debug>:-g> $<$<CONFIG:Release>:-O3 -fno-math-errno -fno-trapping-math -fno-rtti -fvisibility=hidden -fomit-frame-pointer>)


target_compile_features(gd-sim PRIVATE cxx_std_20)
target_include_directories(gd-sim PUBLIC include)


add_executable(gd-sim-test ${SOURCES} test/test.cpp)

set_target_properties(gd-sim-test PROPERTIES INTERPROCEDURAL_OPTIMIZATION 0)
target_include_directories(gd-sim-test PRIVATE include)