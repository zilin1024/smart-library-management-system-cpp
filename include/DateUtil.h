#ifndef DATE_UTIL_H
#define DATE_UTIL_H

#include <chrono>
#include <string>

struct Date {
    int year;
    int month;
    int day;

    Date();
    Date(int y, int m, int d);

    bool operator==(const Date& other) const;
    bool operator!=(const Date& other) const;
    bool operator<(const Date& other) const;
    bool operator<=(const Date& other) const;
    bool operator>(const Date& other) const;
    bool operator>=(const Date& other) const;
};

namespace DateUtil {
    Date today();
    Date fromString(const std::string& text, char delimiter = '-');
    std::string toString(const Date& date, char delimiter = '-');
    int daysBetween(const Date& start, const Date& end);
    Date addDays(const Date& date, int days);
    bool isValid(const Date& date);
    Date fromTimePoint(const std::chrono::system_clock::time_point& tp);
    std::chrono::system_clock::time_point toTimePoint(const Date& date);

    // === 新增工具函数 ===
    int currentHour();                 // 返回本地时间的小时数（0-23）
    long long toSerial(const Date& date); // 将日期转换为自 1970-01-01 起的日序数
}

#endif // DATE_UTIL_H
