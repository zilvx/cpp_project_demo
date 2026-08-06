#ifndef LEETCODE_PROBLEM_SOLVE_H
#define LEETCODE_PROBLEM_SOLVE_H 

#include <iostream>
#include <string>
#include <vector>

class LeetCodeProblemSolve
{
public:
    LeetCodeProblemSolve();
    ~LeetCodeProblemSolve();

    static bool isAnagrams(std::string str1, std::string str2);

    std::vector<std::vector<std::string>> groupAnagrams(std::vector<std::string>& strs);
};

#endif // LEETCODE_PROBLEM_SOLVE_H