add_library(lcps_lib LeetCodeProblemSolve.cpp)

set_target_properties(lcps_lib PROPERTIES
    PUBLIC_HEADER LeetCodeProblemSolve.h
)
target_include_directories(lcps_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})