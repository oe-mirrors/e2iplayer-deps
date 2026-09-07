
#ifndef _STRING_HELPER_H_
#define _STRING_HELPER_H_


#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>

#include "SimpleDataTypes.h"

class CStringHelper
{
public:
    template <typename T>
    static T aton(const std::string &strNum)
    {
        T numb = 0;
        std::istringstream ( strNum ) >> numb;
        return numb;
    }
    
    static bool is_number(const std::string &inStr);
    static void splitString(const std::string &inStr, std::vector<std::string> &outVect,
                            const char delim, const bool addEmpty);
    static std::string trim(const std::string &str, const std::string &whitespace = " \t\n\r");
    static bool startsWith(const std::string& haystack, const std::string& needle);
    static void replaceAll(std::string& str, const std::string& from, const std::string& to);
    static void replace(std::string& str, const std::string& from, const std::string& to);
};

#endif //_STRING_HELPER_H_
