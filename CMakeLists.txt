cmake_minimum_required(VERSION 3.10)
project(TEST_CPP_PROJECT VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)  # 必须设置
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
add_executable(main
    src/random/TestRandom.cpp
    src/main.cpp
)