#include "LongestPalindrome.h"

LongestPalindrome::LongestPalindrome() {}

LongestPalindrome::~LongestPalindrome() {}  

bool LongestPalindrome::isPalindrome(const std::string &s) {
    int left = 0, right = s.size() - 1;
    while (left < right) {
        if (s[left] != s[right]) {
            return false;
        }
        left++;
        right--;
    }
    return true;
}

std::string LongestPalindrome::longestPalindrome(const std::string &s) {
    if (s.empty()) return "";

    int start = 0, maxLength = 1;
    for (int i = 0; i < s.size(); i++) {
        // Check for odd-length palindromes
        int left = i, right = i;
        while (left >= 0 && right < s.size() && s[left] == s[right]) {
            if (right - left + 1 > maxLength) {
                start = left;
                maxLength = right - left + 1;
            }
            left--;
            right++;
        }

        // Check for even-length palindromes
        left = i, right = i + 1;
        while (left >= 0 && right < s.size() && s[left] == s[right]) {
            if (right - left + 1 > maxLength) {
                start = left;
                maxLength = right - left + 1;
            }
            left--;
            right++;
        }
    }
    return s.substr(start, maxLength);
}