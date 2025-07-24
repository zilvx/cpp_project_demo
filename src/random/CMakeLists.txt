# 定义TestRandom库
add_library(random_lib TestRandom.cpp)

# 设置include目录， 这样可以直接使用#include "TestRandom.h"而不需要指定路径
set_target_properties(random_lib PROPERTIES
    PUBLIC_HEADER TestRandom.h
)

target_link_libraries(random_lib PUBLIC
    spdlog::spdlog
)

target_include_directories(random_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})