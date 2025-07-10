cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()


set(CMAKE_GENERATOR "Unix Makefiles")

project(myTinyMuduo)

# 添加子项目
add_subdirectory(./mymuduo)
add_subdirectory(./examples)
add_subdirectory(./tests)
add_subdirectory(./benchmarks)