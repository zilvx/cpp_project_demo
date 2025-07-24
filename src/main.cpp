#include <iostream>
#include <spdlog/spdlog.h>
#include "random/TestRandom.h"
#include "leetcode/LongestPalindrome/LongestPalindrome.h"
#include "spdlog/logutil.h"

int main() {
    LogUtil::init();
    spdlog::info("Hello, VSCode C++ on macOS!");

// #ifdef UNUSED
    initRandomNumByCStyle();
    initRandomNumByCpp("int");
    initRandomNumByCpp("real"); 
    initRandomNumByCpp("normal");
    spdlog::info("Random number generation tests completed.");
// #else
    LongestPalindrome lp;
    std::string testStr = "babad";
    std::string longestPalin = lp.longestPalindrome(testStr);
    spdlog::info("Longest Palindrome in '{}' is: '{}'", testStr, longestPalin);
// #endif
    LogUtil::shutdown();
    return 0;
}

