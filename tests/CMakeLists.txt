# ====== Unit Tests (GoogleTest via Conan) ======
find_package(GTest REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Test)

set(TEST_SRCS
    main_test.cpp
    test_nice_step.cpp
    test_waveform_data.cpp
    test_table_data.cpp
    test_edit_controller.cpp
)

# 被测源文件（直接编译进测试可执行文件）
set(SRCS_UNDER_TEST
    ${CMAKE_SOURCE_DIR}/src/ui/core/WaveformData.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/core/EditController.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/WaveformChart.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/core/ScrollBar.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/core/PenIconDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/TableWidget.cpp
)

add_executable(table_tests ${TEST_SRCS} ${SRCS_UNDER_TEST})

target_include_directories(table_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ui/core
    ${CMAKE_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_SOURCE_DIR}/src/ui/providers
)

target_link_libraries(table_tests PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
    spdlog::spdlog
)

# 链接 stub AGL（macOS 26+ 兼容，需同时设置 -F 和 rpath）
target_link_options(table_tests PRIVATE
    -F${CMAKE_SOURCE_DIR}/stub_frameworks
    -Wl,-rpath,${CMAKE_SOURCE_DIR}/stub_frameworks
)

set_target_properties(table_tests PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE FALSE
)

if(USE_PROTO_DATA)
    target_compile_definitions(table_tests PRIVATE USE_PROTO_DATA)
    target_sources(table_tests PRIVATE ${PROTO_SRCS})
    target_include_directories(table_tests PRIVATE ${CMAKE_BINARY_DIR})
    target_link_libraries(table_tests PRIVATE protobuf::libprotobuf)
endif()

# 注册到 CTest
include(GoogleTest)
gtest_discover_tests(table_tests)
