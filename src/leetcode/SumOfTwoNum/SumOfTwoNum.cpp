#include "SumOfTwoNum.h"

SumOfTwoNum::SumOfTwoNum() {}

SumOfTwoNum::~SumOfTwoNum() {}

std::vector<int> SumOfTwoNum::twoSum(const std::vector<int>& nums, int target) {
    std::vector<int> result;
    for (int i = 0; i < nums.size() - 1; i++) {
        for (int j = i + 1; j < nums.size(); j++) {
            if (target == nums.at(i) + nums.at(j)) {
                result.emplace_back(i);
                result.emplace_back(j);
                return result;
            }
        }

    }
    return result;
}