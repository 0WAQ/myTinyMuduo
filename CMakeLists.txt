cmake_minimum_required(VERSION 3.10.0)

# 项目名称
project(myTinyMuduo)

# 打开编译详细过程
set(CMAKE_VERBOSE_MAKEFILE on)

# cpp编译器与编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O1")
set(CMAKE_CXX_STANDARD 17)

# 添加子项目
add_subdirectory(./src)
add_subdirectory(./example)