#include "LeetCodeProblemSolve.h"

LeetCodeProblemSolve::LeetCodeProblemSolve() {}

LeetCodeProblemSolve::~LeetCodeProblemSolve() {}

static bool isAnagrams(std::string str1, std::string str2) {
    // 元素完全相同 且 长度一致
    if (str1.length() == str2.length()) {
        if (str1.compare("") == 0) {
            return true;
        }
        for (int i = 0; i < str1.length(); i++) {
            if (str2.find(str1.at(i)) == -1) {
                return false;
            }
        }

        return true;
    }
    return false;
}

std::vector<std::vector<std::string>> LeetCodeProblemSolve::groupAnagrams(std::vector<std::string>& strs) {
    std::vector<std::vector<std::string>> result;
    if (strs.size() == 1) {
        result.emplace_back(strs);
        return result;
    }

    for (int i = 0; i < strs.size() - 1; i++) {
        std::vector<std::string> temp;
        for (int j = 0; j < i + 1; j++) {
            if (isAnagrams(strs.at(i), strs.at(j))) {
                

            }
    }

}

