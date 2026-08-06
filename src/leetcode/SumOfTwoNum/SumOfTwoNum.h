#ifndef SUM_OF_TWO_NUM
#define SUM_OF_TWO_NUM 

#include <iostream>
#include <string>
#include <vector>

class SumOfTwoNum
{
public:
    SumOfTwoNum();
    ~SumOfTwoNum();

    std::vector<int> twoSum(const std::vector<int>& nums, int target);
};

#endif // SUM_OF_TWO_NUM
