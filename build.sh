#!/bin/bash

set -e

# 创建build目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 创建lib与bin目录
if [ ! -d `pwd`/lib ]; then
    mkdir `pwd`/lib
fi

if [ ! -d `pwd`/bin ]; then
    mkdir `pwd`/bin
fi

rm -rf `pwd`/build/*

cd `pwd`/build && 
    cmake .. && 
    make -j${nproc}

# 回到项目根目录
cd ..
