cmake_minimum_required(VERSION 3.10)
project(TEST_CPP_PROJECT VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# 构建类型默认为 Release（可用 -DCMAKE_BUILD_TYPE=Debug 覆盖）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Release or Debug)" FORCE)
endif()

include(${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake)

find_package(spdlog REQUIRED)

# ====== Qt6 集成 ======
# Homebrew 安装的 Qt6 路径
list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
find_package(Qt6 REQUIRED COMPONENTS Widgets)
# ======================

# 添加子目录, CMake会自动处理该目录下的CMakeLists.txt文件
add_subdirectory(src/spdlog)
add_subdirectory(src/random)
add_subdirectory(src/leetcode)

# 控制台应用程序 (spdlog + leetcode)
add_executable(main src/random/TestRandom.cpp src/main.cpp)
target_link_libraries(main PRIVATE
    spdlog::spdlog
    lp_lib
    sum_lib
    random_lib
    logutil_lib
)

# Qt 表格应用程序
add_executable(qt_table_app
    src/qt_widget/main_qt.cpp
    src/qt_widget/TableWidget.cpp
    src/qt_widget/ScrollBar.cpp
    src/qt_widget/PenIconDelegate.cpp
    src/qt_widget/VirtualKeyboard.cpp
    src/qt_widget/EditController.cpp
    src/qt_widget/resources.qrc
)
target_link_libraries(qt_table_app PRIVATE Qt6::Widgets)
# 链接时使用 stub AGL 框架 (macOS SDK 已移除 AGL)
target_link_options(qt_table_app PRIVATE
    -F${CMAKE_SOURCE_DIR}/stub_frameworks
    -Wl,-rpath,@executable_path/../Frameworks
)

# 构建后将 stub AGL 嵌入 .app bundle，解决可移植性问题
add_custom_command(TARGET qt_table_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_CONTENT_DIR:qt_table_app>/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/stub_frameworks"
        "$<TARGET_BUNDLE_CONTENT_DIR:qt_table_app>/Frameworks"
    COMMENT "Embedding stub AGL framework into app bundle"
)

set_target_properties(qt_table_app PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

