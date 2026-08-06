#include <iostream>
#include <spdlog/spdlog.h>
#include "random/TestRandom.h"
#include "leetcode/LongestPalindrome/LongestPalindrome.h"
#include "leetcode/SumOfTwoNum/SumOfTwoNum.h"
#include "spdlog/logutil.h"

int main() {
    LogUtil::init();
    spdlog::info("Hello, VSCode C++ on macOS!");

    initRandomNumByCStyle();
    initRandomNumByCpp("int");
    initRandomNumByCpp("real");
    initRandomNumByCpp("normal");
    spdlog::info("Random number generation tests completed.");

    LongestPalindrome lp;
    std::string testStr = "babad";
    std::string longestPalin = lp.longestPalindrome(testStr);
    spdlog::info("Longest Palindrome in '{}' is: '{}'", testStr, longestPalin);

    SumOfTwoNum stn;
    std::vector<int> stnResult = stn.twoSum({2, 7, 11, 15}, 9);
    spdlog::info("Sum Of Two Num Result is '{}' and '{}'", stnResult.at(0), stnResult.at(1));

    LogUtil::shutdown();
    return 0;
}

