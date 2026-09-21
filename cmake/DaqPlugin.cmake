# DaqPlugin.cmake — InsightDAQ 插件 CMake 辅助脚本
#
# 用法：
#   include(cmake/DaqPlugin.cmake)
#   daq_plugin_setup(MyPlugin)
#
# 效果：
#   - 自动添加 SDK include 路径
#   - 自动设置符号隐藏
#   - 自动链接 Qt6::Core + Qt6::Gui
#   - 自动编译 DaqIO.cpp

function(daq_plugin_setup target)
    # SDK 路径（默认为 cmake/ 上级目录）
    if(NOT DEFINED DAQ_SDK_DIR)
        set(DAQ_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "DAQ SDK root directory")
    endif()

    message(STATUS "[DaqPlugin] SDK 路径: ${DAQ_SDK_DIR}")
    message(STATUS "[DaqPlugin] 插件目标: ${target}")

    # 添加 SDK include 路径
    target_include_directories(${target} PRIVATE
        ${DAQ_SDK_DIR}/include
    )

    # 编译 DaqIO.cpp（SDK 自包含版本，无主程序依赖）
    target_sources(${target} PRIVATE
        ${DAQ_SDK_DIR}/include/DaqIO.cpp
    )

    # 符号隐藏（只导出 DAQ_PLUGIN_EXPORT 标记的符号）
    set_target_properties(${target} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    # 链接 Qt6::Core + Qt6::Gui（QImage/QPainter 绘图需要）
    find_package(Qt6 REQUIRED COMPONENTS Core Gui)
    target_link_libraries(${target} PRIVATE Qt6::Core Qt6::Gui)

    # 输出文件名：lib + 项目名
    set_target_properties(${target} PROPERTIES
        PREFIX "lib"
    )

    message(STATUS "[DaqPlugin] 配置完成: ${target}")
endfunction()
