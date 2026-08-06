# 定义TestRandom库
add_library(logutil_lib logutil.cpp)

# 链接spdlog头文件库
target_link_libraries(logutil_lib PRIVATE
    spdlog::spdlog
)

# 设置include目录， 这样可以直接使用#include "TestRandom.h"而不需要指定路径
set_target_properties(logutil_lib PROPERTIES
    PUBLIC_HEADER logutil.h
)
target_include_directories(logutil_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})