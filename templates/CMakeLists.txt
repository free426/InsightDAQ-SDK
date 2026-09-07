cmake_minimum_required(VERSION 3.16)
project(MyPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# SDK 路径（根据实际位置调整）
set(DAQ_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk" CACHE PATH "DAQ SDK 路径")

# 引入 SDK 辅助脚本
include(${DAQ_SDK_DIR}/cmake/DaqPlugin.cmake)

# 创建插件库
add_library(${PROJECT_NAME} SHARED
    src/MyPlugin.cpp
)

# 一行配置：include 路径 + 符号隐藏 + Qt 链接 + DaqIO 编译
daq_plugin_setup(${PROJECT_NAME})
