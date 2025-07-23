#include <iostream>
// #include <spdlog/spdlog.h>
#include "random/TestRandom.h"
#include "leetcode/LongestPalindrome/LongestPalindrome.h"

int main() {
    // spdlog::info("Hello, VSCode C++ on macOS!");
    std::cout << "Hello, VSCode C++ on macOS!" << std::endl;

#ifdef UNUSED
    initRandomNumByCStyle();
    initRandomNumByCpp("int");
    initRandomNumByCpp("real");
    initRandomNumByCpp("normal");

#else
    LongestPalindrome lp;
    std::string testStr = "babad";
    std::string longestPalin = lp.longestPalindrome(testStr);
    std::cout << "Longest Palindrome in '" << testStr << "' is: '" << longestPalin << "'" << std::endl;
#endif

    return 0;
}

