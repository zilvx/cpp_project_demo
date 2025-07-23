# 定义TestRandom库
add_library(TestRandom_lib TestRandom.cpp)

# 设置include目录， 这样可以直接使用#include "TestRandom.h"而不需要指定路径
set_target_properties(TestRandom_lib PROPERTIES
    PUBLIC_HEADER TestRandom.h
)
target_include_directories(TestRandom_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})