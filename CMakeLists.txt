cmake_minimum_required(VERSION 3.10.0)

# 项目名称
project(myTinyMuduo)

# 打开编译详细过程
set(CMAKE_VERBOSE_MAKEFILE off)

# cpp编译器与编译选项
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_STANDARD 20)

# 添加子项目
add_subdirectory(./mymuduo)
add_subdirectory(./example)
add_subdirectory(./tests)