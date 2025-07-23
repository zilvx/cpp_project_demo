# 定义LongestPalindrome库
add_library(LongestPalindrome_lib LongestPalindrome.cpp)

# 设置include目录， 这样可以直接使用#include "LongestPalindrome.h"而不需要指定路径
set_target_properties(LongestPalindrome_lib PROPERTIES
    PUBLIC_HEADER LongestPalindrome.h
)
target_include_directories(LongestPalindrome_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})