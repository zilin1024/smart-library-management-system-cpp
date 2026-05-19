#include "StringUtil.h"
#include <sstream>

namespace StringUtil {

    namespace {
        // 内部辅助函数：转换字符串大小写
        std::string transformCase(const std::string& text, bool toLowerCase) {
            std::string result(text.size(), '\0');
            std::locale loc;
            std::transform(text.begin(), text.end(), result.begin(), [&](unsigned char ch) {
                return toLowerCase ? std::tolower(ch, loc) : std::toupper(ch, loc);
                });
            return result;
        }
    }

    // 去除字符串首尾的空白字符
    std::string trim(const std::string& text) {
        // 查找第一个非空白字符
        auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
            return std::isspace(ch);
            });

        // 查找最后一个非空白字符（反向迭代）
        auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
            return std::isspace(ch);
            }).base();

        if (begin >= end) {
            return {}; // 全是空白字符，返回空字符串
        }

        return std::string(begin, end);
    }

    // 转小写
    std::string toLower(const std::string& text) {
        return transformCase(text, true);
    }

    // 转大写
    std::string toUpper(const std::string& text) {
        return transformCase(text, false);
    }

    // 忽略大小写比较是否相等
    bool equalsIgnoreCase(const std::string& lhs, const std::string& rhs) {
        return toLower(lhs) == toLower(rhs);
    }

    // 忽略大小写检查包含关系
    bool containsIgnoreCase(const std::string& text, const std::string& pattern) {
        if (pattern.empty()) {
            return true;
        }
        auto lText = toLower(text);
        auto lPattern = toLower(pattern);
        return lText.find(lPattern) != std::string::npos;
    }

    // 按指定分隔符分割字符串
    std::vector<std::string> split(const std::string& text, char delimiter) {
        std::vector<std::string> result;
        std::istringstream iss(text);
        std::string token;
        while (std::getline(iss, token, delimiter)) {
            result.push_back(token);
        }
        return result;
    }

    // 使用指定分隔符连接字符串列表
    std::string join(const std::vector<std::string>& parts, const std::string& delimiter) {
        if (parts.empty()) {
            return {};
        }

        std::ostringstream oss;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) {
                oss << delimiter;
            }
            oss << parts[i];
        }
        return oss.str();
    }

    // 替换所有出现的子串
    std::string replaceAll(const std::string& text, const std::string& from, const std::string& to) {
        if (from.empty()) {
            return text;
        }

        std::string result = text;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size(); // 跳过替换后的内容，避免死循环
        }
        return result;
    }

    // 检查是否以指定前缀开头
    bool startsWith(const std::string& text, const std::string& prefix, bool ignoreCase) {
        if (prefix.size() > text.size()) {
            return false;
        }

        auto left = text.substr(0, prefix.size());
        return ignoreCase ? equalsIgnoreCase(left, prefix) : left == prefix;
    }

    // 检查是否以指定后缀结尾
    bool endsWith(const std::string& text, const std::string& suffix, bool ignoreCase) {
        if (suffix.size() > text.size()) {
            return false;
        }

        auto right = text.substr(text.size() - suffix.size());
        return ignoreCase ? equalsIgnoreCase(right, suffix) : right == suffix;
    }

} 
