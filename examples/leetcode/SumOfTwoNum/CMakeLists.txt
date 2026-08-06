add_library(sum_lib SumOfTwoNum.cpp)

set_target_properties(sum_lib PROPERTIES
    PUBLIC_HEADER SumOfTwoNum.h
)
target_include_directories(sum_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})