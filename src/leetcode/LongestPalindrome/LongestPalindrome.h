#ifndef LONGEST_PALINDROME_H
#define LONGEST_PALINDROME_H 

#include <iostream>
#include <string>
#include <vector>

class LongestPalindrome
{
public:
    LongestPalindrome();
    ~LongestPalindrome();

    bool isPalindrome(const std::string &s);
    std::string longestPalindrome(const std::string &s);
};

#endif // LONGEST_PALINDROME_H
