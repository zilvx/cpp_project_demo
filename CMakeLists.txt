cmake_minimum_required(VERSION 3.10)
project(TEST_CPP_PROJECT VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)  # 必须设置

include(${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)

find_package(spdlog REQUIRED)

# 设置头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/spdlog)
include_directories(${PROJECT_SOURCE_DIR}/random)
include_directories(${PROJECT_SOURCE_DIR}/leetcode)

# 添加子目录, CMake会自动处理该目录下的CMakeLists.txt文件
add_subdirectory(src/spdlog)
add_subdirectory(src/random)
add_subdirectory(src/leetcode)

add_executable(main src/random/TestRandom.cpp src/main.cpp)
# 链接库
target_link_libraries(main PRIVATE 
    spdlog::spdlog
    lp_lib
    random_lib
    logutil_lib
)

