cmake_minimum_required(VERSION 3.10)
project(TEST_CPP_PROJECT VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

option(USE_HTTP_DATA  "Use HTTP JSON backend for table data" OFF)
option(USE_PROTO_DATA "Use HTTP Protobuf backend for table data" OFF)

if(USE_HTTP_DATA AND USE_PROTO_DATA)
    message(FATAL_ERROR "USE_HTTP_DATA and USE_PROTO_DATA are mutually exclusive.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake)
find_package(spdlog REQUIRED)

# ====== Qt6 ======
list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

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
    src/ui/providers/IDataProvider.cpp
    src/ui/providers/SampleDataProvider.cpp
    src/ui/providers/HttpDataProvider.cpp
    src/ui/widgets/WaveformChart.cpp
    src/ui/resources.qrc
)
target_link_libraries(qt_table_app PRIVATE Qt6::Widgets Qt6::Network)
target_link_options(qt_table_app PRIVATE
    -F${CMAKE_SOURCE_DIR}/stub_frameworks
    -Wl,-rpath,@executable_path/../Frameworks
)
add_custom_command(TARGET qt_table_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_CONTENT_DIR:qt_table_app>/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/stub_frameworks"
        "$<TARGET_BUNDLE_CONTENT_DIR:qt_table_app>/Frameworks"
    COMMENT "Embedding stub AGL framework into app bundle"
)
set_target_properties(qt_table_app PROPERTIES
    MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE AUTOMOC ON AUTORCC ON AUTOUIC ON
)

if(USE_HTTP_DATA)
    target_compile_definitions(qt_table_app PRIVATE USE_HTTP_DATA)
endif()

# ====== Protobuf (optional) ======
if(USE_PROTO_DATA)
    list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/protobuf")
    find_package(Protobuf MODULE REQUIRED)
    set(PROTO_FILE ${CMAKE_SOURCE_DIR}/src/utils/proto/table_data.proto)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})
    target_sources(qt_table_app PRIVATE
        src/ui/providers/HttpProtoDataProvider.cpp ${PROTO_SRCS})
    target_compile_definitions(qt_table_app PRIVATE USE_PROTO_DATA)
    target_include_directories(qt_table_app PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR} /opt/homebrew/opt/abseil/include)
    target_link_libraries(qt_table_app PRIVATE protobuf::libprotobuf)
endif()

# ====== HTTP Backend Server ======
add_executable(table_data_server
    src/bs/main_bs.cpp
    src/bs/TableDataServer.cpp
)
target_link_libraries(table_data_server PRIVATE Qt6::Network)
set_target_properties(table_data_server PROPERTIES AUTOMOC ON)

if(USE_PROTO_DATA)
    target_sources(table_data_server PRIVATE ${PROTO_SRCS})
    target_compile_definitions(table_data_server PRIVATE TABLE_DATA_SERVER_PROTO)
    target_include_directories(table_data_server PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR} /opt/homebrew/opt/abseil/include)
    target_link_libraries(table_data_server PRIVATE protobuf::libprotobuf)
endif()

