cmake_minimum_required(VERSION 3.10)
project(TEST_CPP_PROJECT VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# ====== 自动加载 Conan 工具链（未显式传入或传入路径已失效时） ======
# 使 cmake -S . -B build -DCMAKE_BUILD_TYPE=Release 无需手动带
# -DCMAKE_TOOLCHAIN_FILE 也能配置成功。若已在命令行显式传入有效工具链则跳过。
if(NOT CMAKE_TOOLCHAIN_FILE OR NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    file(GLOB_RECURSE _conan_toolchains
        LIST_DIRECTORIES FALSE
        "${CMAKE_CURRENT_SOURCE_DIR}/build/**/conan_toolchain.cmake"
    )
    list(SORT _conan_toolchains)
    if(_conan_toolchains)
        list(GET _conan_toolchains 0 _conan_toolchain)
        message(STATUS "Conan toolchain: ${_conan_toolchain}")
        include("${_conan_toolchain}")
    else()
        message(FATAL_ERROR
            "未找到 Conan 生成的 conan_toolchain.cmake。请先运行:\n"
            "  conan install . --output-folder=. --build=missing -s build_type=Release")
    endif()
endif()
option(USE_HTTP_DATA  "Use HTTP JSON backend for table data" OFF)
option(USE_PROTO_DATA "Use HTTP Protobuf backend for table data" OFF)

if(USE_HTTP_DATA AND USE_PROTO_DATA)
    message(FATAL_ERROR "USE_HTTP_DATA and USE_PROTO_DATA are mutually exclusive.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(spdlog REQUIRED)

# ====== Qt6 ======
list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
find_package(Qt6 REQUIRED COMPONENTS Widgets Network QuickWidgets)

# ====== Sub-libraries ======
add_subdirectory(src/utils/logging)
add_subdirectory(examples/random)
add_subdirectory(examples/leetcode)

# ====== Console app ======
add_executable(main examples/random/TestRandom.cpp main.cpp)
target_link_libraries(main PRIVATE spdlog::spdlog lp_lib sum_lib random_lib logutil_lib)

# ====== Qt Table App ======
add_executable(qt_table_app
    src/ui/main_ui.cpp
    src/ui/widgets/TableWidget.cpp
    src/ui/widgets/VirtualKeyboard.cpp
    src/ui/core/ScrollBar.cpp
    src/ui/core/PenIconDelegate.cpp
    src/ui/core/EditController.cpp
    src/ui/core/WaveformData.cpp
    src/ui/core/WaveformGenerator.cpp
    src/ui/providers/IDataProvider.cpp
    src/ui/providers/SampleDataProvider.cpp
    src/ui/providers/HttpDataProvider.cpp
    src/ui/widgets/WaveformChart.cpp
    src/ui/widgets/ArbWidget.cpp
    src/ui/widgets/FileManagerWidget.cpp
    src/ui/widgets/FileManagerDialog.cpp
    src/ui/widgets/FiveGNrWidget.cpp
    src/ui/core/ToastService.h
    src/ui/components/ui_common/param_row.cpp
    src/ui/components/ui_common/led_indicator.cpp
    src/ui/components/ui_common/stat_row.cpp
    src/ui/components/ui_common/group_box.cpp
    src/ui/components/ui_common/nav_button.cpp
    src/ui/components/ui_common/lcd_display.cpp
    src/ui/components/ui_common/keyboard_pad.cpp
    src/ui/resources.qrc
    src/ui/qml.qrc
)
target_link_libraries(qt_table_app PRIVATE Qt6::Widgets Qt6::Network Qt6::QuickWidgets spdlog::spdlog logutil_lib)
# 链接 stub AGL（macOS 26+ 移除 AGL framework，Qt 仍链接它，需 -F + rpath）
target_link_options(qt_table_app PRIVATE
    -F${CMAKE_SOURCE_DIR}/stub_frameworks
    -Wl,-rpath,${CMAKE_SOURCE_DIR}/stub_frameworks
)
set_target_properties(qt_table_app PROPERTIES
    MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE AUTOMOC ON AUTORCC ON AUTOUIC ON
    # 空 CFBundleIdentifier 会导致 macOS 原生 NSOpenPanel/NSSavePanel 静默失败
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.cppprojectdemo.qt-table-app"
    MACOSX_BUNDLE_BUNDLE_NAME "Qt Table App"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
)

if(USE_HTTP_DATA)
    target_compile_definitions(qt_table_app PRIVATE USE_HTTP_DATA)
endif()

# ====== 多进程抓取: 子进程程序（网络请求 + JSON 解析，独立进程） ======
add_executable(fetch_table_child
    src/child/fetch_table_child.cpp
)
target_link_libraries(fetch_table_child PRIVATE Qt6::Network Qt6::Core spdlog::spdlog logutil_lib)

# ====== 多进程抓取: 演示应用（ForkedHttpDataProvider + TableWidget） ======
add_executable(qt_table_app_fork
    src/ui/main_forked_demo.cpp
    src/ui/providers/ForkedHttpDataProvider.cpp
    src/ui/widgets/TableWidget.cpp
    src/ui/widgets/VirtualKeyboard.cpp
    src/ui/core/ScrollBar.cpp
    src/ui/core/PenIconDelegate.cpp
    src/ui/core/EditController.cpp
    src/ui/core/WaveformData.cpp
    src/ui/core/WaveformGenerator.cpp
    src/ui/providers/IDataProvider.cpp
    src/ui/providers/SampleDataProvider.cpp
    src/ui/providers/HttpDataProvider.cpp
    src/ui/widgets/WaveformChart.cpp
    src/ui/widgets/ArbWidget.cpp
    src/ui/widgets/FileManagerWidget.cpp
    src/ui/widgets/FileManagerDialog.cpp
    src/ui/widgets/FiveGNrWidget.cpp
    src/ui/core/ToastService.h
    src/ui/components/ui_common/param_row.cpp
    src/ui/components/ui_common/led_indicator.cpp
    src/ui/components/ui_common/stat_row.cpp
    src/ui/components/ui_common/group_box.cpp
    src/ui/components/ui_common/nav_button.cpp
    src/ui/components/ui_common/lcd_display.cpp
    src/ui/components/ui_common/keyboard_pad.cpp
    src/ui/resources.qrc
)
target_link_libraries(qt_table_app_fork PRIVATE Qt6::Widgets Qt6::Network spdlog::spdlog logutil_lib)
set_target_properties(qt_table_app_fork PROPERTIES
    MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE AUTOMOC ON AUTORCC ON AUTOUIC ON
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.cppprojectdemo.qt-table-app-fork"
    MACOSX_BUNDLE_BUNDLE_NAME "Qt Table App Fork"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
)
# 链接 stub AGL（macOS 26+ 兼容，与 tests/CMakeLists.txt 一致）
target_link_options(qt_table_app_fork PRIVATE
    -F${CMAKE_SOURCE_DIR}/stub_frameworks
    -Wl,-rpath,${CMAKE_SOURCE_DIR}/stub_frameworks
)

# ====== HTTP Backend Server (cpp-httplib) ======
find_package(httplib REQUIRED)
add_executable(table_data_server
    src/bs/main_bs.cpp
    src/bs/TableDataServer.cpp
)
target_link_libraries(table_data_server PRIVATE Qt6::Core httplib::httplib spdlog::spdlog logutil_lib)
set_target_properties(table_data_server PROPERTIES AUTOMOC ON)

# ====== Prefork 多进程 HTTP 服务器（纯 POSIX，不依赖 Qt/cpp-httplib） ======
add_executable(table_data_server_prefork
    src/bs/main_prefork.cpp
    src/bs/PreforkServer.cpp
)
target_link_libraries(table_data_server_prefork PRIVATE spdlog::spdlog logutil_lib)

# ====== Protobuf (optional, via Conan) ======
if(USE_PROTO_DATA)
    find_package(protobuf REQUIRED)

    set(PROTO_FILE ${CMAKE_SOURCE_DIR}/src/utils/proto/table_data.proto)
    set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR})

    # configure 阶段生成 proto 文件（AUTOMOC 编译 mocs_compilation.cpp 时需要 .pb.h）
    get_target_property(_PROTOC protobuf::protoc IMPORTED_LOCATION)
    execute_process(
        COMMAND ${_PROTOC}
            --cpp_out=${PROTO_GEN_DIR}
            -I${CMAKE_SOURCE_DIR}/src/utils/proto
            ${PROTO_FILE}
        RESULT_VARIABLE _proto_ret
    )
    if(NOT _proto_ret EQUAL 0)
        message(FATAL_ERROR "protoc failed on ${PROTO_FILE}")
    endif()

    set(PROTO_SRCS ${PROTO_GEN_DIR}/table_data.pb.cc ${PROTO_GEN_DIR}/table_data.pb.h)

    target_sources(qt_table_app PRIVATE
        src/ui/providers/HttpProtoDataProvider.cpp ${PROTO_SRCS})
    target_compile_definitions(qt_table_app PRIVATE USE_PROTO_DATA)
    target_include_directories(qt_table_app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(qt_table_app PRIVATE protobuf::libprotobuf)

    target_sources(table_data_server PRIVATE ${PROTO_SRCS})
    target_compile_definitions(table_data_server PRIVATE TABLE_DATA_SERVER_PROTO)
    target_include_directories(table_data_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(table_data_server PRIVATE protobuf::libprotobuf)
endif()

# ====== Unit Tests (GoogleTest via Conan) ======
enable_testing()
add_subdirectory(tests)

