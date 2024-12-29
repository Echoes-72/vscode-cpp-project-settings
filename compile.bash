#!/bin/bash

# 设置调试标志为
DebugFlag=""
# 设置源代码文件路径、包含路径和输出文件名
src="./*.cpp"
include="./include"
exe="main"

# 解析命令行参数
while getopts "g" arg
do
    case $arg in
        g)
            DebugFlag="-g"
            echo "Debug mode"
            ;;
        ?)
            echo "Usage: compile.bash [-g]"
            exit 1
            ;;
    esac
done

# 设置调试标志

# 使用 clang++ 编译源代码
clang++ $src -I $include $DebugFlag -D_UNICODE -DUNICODE -o ./bin/$exe -std=c++17

# 如果编译成功且没有调试模式，则运行生成的可执行文件
if [ $? -eq 0 ] && [  "$DebugFlag" == "" ]; then
    ./bin/$exe
fi