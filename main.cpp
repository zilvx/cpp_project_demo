#include <iostream>
#include <spdlog/spdlog.h>
#include "examples/random/TestRandom.h"
#include "examples/leetcode/LongestPalindrome/LongestPalindrome.h"
#include "examples/leetcode/SumOfTwoNum/SumOfTwoNum.h"
#include "src/utils/logging/logutil.h"

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
    spdlog::info("Longest Palindrome in '{}' is: '{}'", testStr, lp.longestPalindrome(testStr));
    SumOfTwoNum stn;
    std::vector<int> stnResult = stn.twoSum({2, 7, 11, 15}, 9);
    spdlog::info("Sum Of Two Num Result is '{}' and '{}'", stnResult.at(0), stnResult.at(1));
    LogUtil::shutdown();
    return 0;
}
