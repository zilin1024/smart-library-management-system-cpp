#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <algorithm>
#include <cctype>
#include <locale>
#include <string>
#include <vector>

namespace StringUtil {

    std::string trim(const std::string& text);
    std::string toLower(const std::string& text);
    std::string toUpper(const std::string& text);
    bool equalsIgnoreCase(const std::string& lhs, const std::string& rhs);
    bool containsIgnoreCase(const std::string& text, const std::string& pattern);
    std::vector<std::string> split(const std::string& text, char delimiter);
    std::string join(const std::vector<std::string>& parts, const std::string& delimiter);
    std::string replaceAll(const std::string& text, const std::string& from, const std::string& to);
    bool startsWith(const std::string& text, const std::string& prefix, bool ignoreCase = false);
    bool endsWith(const std::string& text, const std::string& suffix, bool ignoreCase = false);

}

#endif // STRING_UTIL_H

