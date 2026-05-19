#include "DateUtil.h"
#include <iomanip>
#include <sstream>
#include <tuple>

namespace {
    // 判断是否为闰年
    bool isLeapYear(int year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    // 获取指定年份和月份的天数
    int daysInMonth(int year, int month) {
        static const int daysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (month == 2 && isLeapYear(year)) {
            return 29;  // 闰年2月有29天
        }
        return daysPerMonth[month - 1];  // 普通月份
    }
}

// ==== Date 结构体 ==========================================================

// 默认构造函数（1970年1月1日）
Date::Date() : year(1970), month(1), day(1) {}

// 带参数构造函数
Date::Date(int y, int m, int d) : year(y), month(m), day(d) {}

// 相等比较运算符
bool Date::operator==(const Date& other) const {
    return year == other.year && month == other.month && day == other.day;
}

// 不等比较运算符
bool Date::operator!=(const Date& other) const {
    return !(*this == other);
}

// 小于比较运算符
bool Date::operator<(const Date& other) const {
    return std::tie(year, month, day) < std::tie(other.year, other.month, other.day);
}

// 小于等于比较运算符
bool Date::operator<=(const Date& other) const {
    return !(*this > other);
}

// 大于比较运算符
bool Date::operator>(const Date& other) const {
    return std::tie(year, month, day) > std::tie(other.year, other.month, other.day);
}

// 大于等于比较运算符
bool Date::operator>=(const Date& other) const {
    return !(*this < other);
}

// ==== DateUtil 命名空间 ===================================================

namespace DateUtil {

    // 获取当前日期
    Date today() {
        return fromTimePoint(std::chrono::system_clock::now());
    }

    // 从字符串解析日期
    Date fromString(const std::string& text, char delimiter) {
        std::istringstream iss(text);
        std::string token;
        int parts[3] = { 0 };
        int index = 0;

        // 按分隔符分割字符串
        while (std::getline(iss, token, delimiter) && index < 3) {
            try {
                parts[index] = std::stoi(token);
            }
            catch (...) {
                return Date();  // 解析失败返回默认日期
            }
            ++index;
        }

        if (index != 3) {
            return Date();  // 格式不正确
        }

        Date parsed(parts[0], parts[1], parts[2]);
        return isValid(parsed) ? parsed : Date();  // 验证日期有效性
    }

    // 将日期转换为字符串
    std::string toString(const Date& date, char delimiter) {
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << date.year
            << delimiter
            << std::setw(2) << std::setfill('0') << date.month
            << delimiter
            << std::setw(2) << std::setfill('0') << date.day;
        return oss.str();
    }

    // 计算两个日期之间的天数差
    int daysBetween(const Date& start, const Date& end) {
        auto tpStart = toTimePoint(start);
        auto tpEnd = toTimePoint(end);
        auto duration = tpEnd - tpStart;
        return static_cast<int>(std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24);
    }

    // 为日期添加指定天数
    Date addDays(const Date& date, int days) {
        auto tp = toTimePoint(date);
        tp += std::chrono::hours(days * 24);
        return fromTimePoint(tp);
    }

    // 验证日期是否有效
    bool isValid(const Date& date) {
        if (date.year < 1 || date.month < 1 || date.month > 12) {
            return false;
        }
        int dim = daysInMonth(date.year, date.month);
        return date.day >= 1 && date.day <= dim;
    }

    // 从时间点转换为日期
    Date fromTimePoint(const std::chrono::system_clock::time_point& tp) {
        std::time_t rawTime = std::chrono::system_clock::to_time_t(tp);
        std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &rawTime);  // Windows安全版本
#else
        localtime_r(&rawTime, &tm);  // POSIX线程安全版本
#endif
        return Date(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    }

    // 从日期转换为时间点
    std::chrono::system_clock::time_point toTimePoint(const Date& date) {
        std::tm tm{};
        tm.tm_year = date.year - 1900;
        tm.tm_mon = date.month - 1;
        tm.tm_mday = date.day;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        std::time_t rawTime = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(rawTime);
    }

    // 获取当前小时（0-23）
    int currentHour() {
        auto now = std::chrono::system_clock::now();
        std::time_t rawTime = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &rawTime);
#else
        localtime_r(&rawTime, &tm);
#endif
        return tm.tm_hour;
    }

    // 将日期转换为序列号（从某个起点开始的天数）
    long long toSerial(const Date& date) {
        auto tp = toTimePoint(date);
        auto duration = tp.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;
    }

} 
