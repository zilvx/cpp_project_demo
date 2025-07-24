# 定义LongestPalindrome库
add_library(lp_lib LongestPalindrome.cpp)

# 设置include目录， 这样可以直接使用#include "LongestPalindrome.h"而不需要指定路径
set_target_properties(lp_lib PROPERTIES
    PUBLIC_HEADER LongestPalindrome.h
)
target_include_directories(lp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})