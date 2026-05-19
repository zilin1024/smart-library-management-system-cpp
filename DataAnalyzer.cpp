#include "DataAnalyzer.h"

#include "DateUtil.h"
#include "StringUtil.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace {
    constexpr int DEFAULT_LOOKBACK_DAYS = 60; // 默认回看 60 天用于预测与统计

    // 将日期转换为序列号
    long long serialOf(const Date& date) {
        return DateUtil::toSerial(date);
    }

    // 确保日期范围的开始和结束顺序正确
    void ensureRange(DateRange& range) {
        if (range.end < range.start) {
            std::swap(range.start, range.end);
        }
    }

    // 限制小时在0-23范围内
    int clampHour(int hour) {
        if (hour < 0) return 0;
        if (hour > 23) return 23;
        return hour;
    }

    // 安全的除法运算，避免除零错误
    double safeDiv(double numerator, double denominator) {
        if (std::abs(denominator) < 1e-9) {
            return 0.0;
        }
        return numerator / denominator;
    }

} // namespace

// 构造函数
DataAnalyzer::DataAnalyzer(BookService& bookService,
    BorrowService& borrowService,
    ReaderService& readerService)
    : m_bookService(bookService),
    m_borrowService(borrowService),
    m_readerService(readerService) {}

// 分析借阅趋势
BorrowTrend DataAnalyzer::analyzeBorrowTrend(DateRange range) {
    ensureRange(range);
    BorrowTrend trend;

    // 收集范围内的记录并统计每日借阅次数
    auto records = collectRecordsInRange(range);
    for (const auto& record : records) {
        trend.dailyCounts[record.getBorrowDate()]++;
    }
    return trend;
}

// 分析借阅高峰时段
PeakHours DataAnalyzer::analyzePeakBorrowHours() {
    PeakHours peak{};
    peak.hourlyCounts.assign(24, 0);  // 初始化24小时的计数数组

    // 分析过去30天的数据
    DateRange range{ DateUtil::addDays(DateUtil::today(), -30), DateUtil::today() };
    ensureRange(range);
    auto records = collectRecordsInRange(range);

    if (records.empty()) {
        peak.peakHour = 0;
        return peak;
    }

    // 统计每小时的借阅次数
    for (const auto& record : records) {
        int hour = clampHour(record.getBorrowHour());
        peak.hourlyCounts[hour] += 1;
    }

    // 找出借阅最多的时段
    peak.peakHour = 0;
    int maxCount = peak.hourlyCounts[0];
    for (int h = 1; h < 24; ++h) {
        if (peak.hourlyCounts[h] > maxCount) {
            maxCount = peak.hourlyCounts[h];
            peak.peakHour = h;
        }
    }
    return peak;
}

// 分析类别受欢迎程度
std::map<std::string, double> DataAnalyzer::analyzeCategoryPopularity() {
    std::map<std::string, double> popularity;

    // 计算每个类别的借阅次数
    auto categoryCount = calculateCategoryBorrowCount();
    int total = 0;
    for (const auto& [category, count] : categoryCount) {
        total += count;
    }
    if (total == 0) {
        return popularity;  // 没有借阅记录
    }

    // 计算每个类别的占比
    for (const auto& [category, count] : categoryCount) {
        popularity[category] = static_cast<double>(count) / static_cast<double>(total);
    }
    return popularity;
}

// 生成读者画像
ReaderProfile DataAnalyzer::generateReaderProfile(const std::string& readerId) {
    ReaderProfile profile{};
    profile.readerId = readerId;

    const Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return profile;  // 读者不存在
    }

    profile.totalBorrowed = reader->getTotalBorrowed();

    // 计算平均借阅时长
    profile.averageBorrowDuration = calculateAverageBorrowDuration(*reader);

    // 统计读者借阅的书籍类别
    std::unordered_map<std::string, int> categoryCounter;
    for (const auto& isbn : reader->getBorrowHistory()) {
        auto books = m_bookService.searchByISBN(isbn);
        if (!books.empty()) {
            categoryCounter[books.front().getCategory()]++;
        }
    }

    // 找出最常借阅的类别（最多3个）
    std::vector<std::pair<std::string, int>> vec(categoryCounter.begin(), categoryCounter.end());
    std::sort(vec.begin(), vec.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;  // 次数相同按字母排序
        }
        return lhs.second > rhs.second;  // 按次数降序
        });

    for (std::size_t i = 0; i < vec.size() && i < 3; ++i) {
        profile.favoriteCategories.push_back(vec[i].first);
    }
    return profile;
}

// 对读者进行分群
std::vector<ReaderSegment> DataAnalyzer::segmentReaders() {
    std::vector<ReaderSegment> segments = {
        {"轻度读者", {} },   // 借阅少于5本
        {"中度读者", {} },   // 借阅5-19本
        {"重度读者", {} }    // 借阅20本及以上
    };

    auto readers = m_readerService.getReadersByBorrowCount(0);
    for (const auto& reader : readers) {
        int total = reader.getTotalBorrowed();
        if (total < 5) {
            segments[0].readerIds.push_back(reader.getReaderId());
        }
        else if (total < 20) {
            segments[1].readerIds.push_back(reader.getReaderId());
        }
        else {
            segments[2].readerIds.push_back(reader.getReaderId());
        }
    }
    return segments;
}

// 计算读者参与度
std::map<std::string, double> DataAnalyzer::calculateReaderEngagement() {
    std::map<std::string, double> engagement;
    auto readers = m_readerService.getReadersByBorrowCount(0);

    Date today = DateUtil::today();
    for (const auto& reader : readers) {
        // 计算活跃天数
        int daysActive = std::max(1, DateUtil::daysBetween(reader.getRegisterDate(), today));
        // 参与度 = 借阅总数 / 活跃天数
        double score = static_cast<double>(reader.getTotalBorrowed()) / static_cast<double>(daysActive);
        engagement[reader.getReaderId()] = score;
    }
    return engagement;
}

// 分析图书利用率
BookUtilization DataAnalyzer::analyzeBookUtilization() {
    BookUtilization utilization{};
    auto books = m_bookService.searchByKeyword("");

    if (books.empty()) {
        return utilization;
    }

    auto popularityMap = m_borrowService.getPopularBooks(static_cast<int>(books.size()) * 2);
    double totalBorrow = 0.0;

    // 遍历所有图书，分类统计
    for (const auto& book : books) {
        auto it = popularityMap.find(book.getISBN());
        int borrowCount = (it != popularityMap.end()) ? it->second : 0;
        totalBorrow += borrowCount;
        if (borrowCount >= 10) {
            utilization.highUtilizationBooks.push_back(book.getISBN());  // 高利用
        }
        else if (borrowCount <= 2) {
            utilization.lowUtilizationBooks.push_back(book.getISBN());   // 低利用
        }
    }

    utilization.averageBorrowPerBook = safeDiv(totalBorrow, books.size());  // 平均借阅次数
    return utilization;
}

// 识别使用不足的书籍（借阅次数低于阈值）
std::vector<std::string> DataAnalyzer::identifyUnderusedBooks(double threshold) {
    std::vector<std::string> result;
    auto popularityMap = m_borrowService.getPopularBooks(0);
    auto books = m_bookService.searchByKeyword("");

    for (const auto& book : books) {
        int count = popularityMap.contains(book.getISBN()) ? popularityMap[book.getISBN()] : 0;
        if (count <= threshold) {
            result.push_back(book.getISBN());
        }
    }
    return result;
}

// 识别过度使用的书籍（借阅次数高于阈值）
std::vector<std::string> DataAnalyzer::identifyOverusedBooks(double threshold) {
    std::vector<std::string> result;
    auto popularityMap = m_borrowService.getPopularBooks(0);
    for (const auto& [isbn, count] : popularityMap) {
        if (count >= threshold) {
            result.push_back(isbn);
        }
    }
    return result;
}

// 预测未来借阅量
BorrowPrediction DataAnalyzer::predictFutureBorrows(int days) {
    BorrowPrediction prediction{};
    if (days <= 0) {
        return prediction;
    }

    auto allRecords = m_borrowService.getAllBorrowRecords();
    if (allRecords.empty()) {
        // 无历史数据，预测为0
        for (int i = 1; i <= days; ++i) {
            prediction.dailyPrediction[DateUtil::addDays(DateUtil::today(), i)] = 0;
        }
        return prediction;
    }

    Date today = DateUtil::today();
    Date startLimit = DateUtil::addDays(today, -DEFAULT_LOOKBACK_DAYS);  // 仅使用最近的数据

    // 统计最近每天的借阅次数
    std::map<long long, int> dailyCount;
    for (const auto& record : allRecords) {
        const Date& borrowDate = record.getBorrowDate();
        if (borrowDate < startLimit || borrowDate > today) {
            continue;  // 超出时间范围
        }
        dailyCount[serialOf(borrowDate)]++;
    }

    // 数据不足时使用简单平均值
    if (dailyCount.size() < 2) {
        int avg = 0;
        if (!dailyCount.empty()) {
            int total = 0;
            for (const auto& item : dailyCount) total += item.second;
            avg = static_cast<int>(std::round(static_cast<double>(total) / dailyCount.size()));
        }
        for (int i = 1; i <= days; ++i) {
            prediction.dailyPrediction[DateUtil::addDays(today, i)] = avg;
        }
        return prediction;
    }

    // 使用线性回归预测
    double n = static_cast<double>(dailyCount.size());
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;

    for (const auto& [serial, count] : dailyCount) {
        sumX += serial;
        sumY += count;
        sumXY += serial * count;
        sumX2 += serial * serial;
    }

    double denominator = n * sumX2 - sumX * sumX;
    double slope = 0.0;
    if (std::abs(denominator) > 1e-9) {
        slope = (n * sumXY - sumX * sumY) / denominator;  // 斜率
    }
    double intercept = (sumY - slope * sumX) / n;  // 截距

    double avg = sumY / n;  // 平均值
    for (int i = 1; i <= days; ++i) {
        Date futureDate = DateUtil::addDays(today, i);
        long long serial = serialOf(futureDate);
        double trendValue = intercept + slope * static_cast<double>(serial);  // 趋势预测值
        double combined = 0.5 * avg + 0.5 * trendValue;  // 结合平均值和趋势
        int predicted = std::max(0, static_cast<int>(std::round(combined)));
        prediction.dailyPrediction[futureDate] = predicted;
    }
    return prediction;
}

// 预测特定书籍的需求
DemandForecast DataAnalyzer::forecastBookDemand(const std::string& isbn) {
    DemandForecast forecast{};
    forecast.isbn = isbn;

    auto allRecords = m_borrowService.getAllBorrowRecords();
    if (allRecords.empty()) {
        forecast.predictedDemand = 0;
        return forecast;
    }

    Date today = DateUtil::today();
    Date startLimit = DateUtil::addDays(today, -DEFAULT_LOOKBACK_DAYS);

    // 统计该书籍最近的借阅情况
    std::map<long long, int> dailyCount;
    for (const auto& record : allRecords) {
        if (record.getISBN() != isbn) {
            continue;  // 不是目标书籍
        }
        const Date& borrowDate = record.getBorrowDate();
        if (borrowDate < startLimit || borrowDate > today) {
            continue;  // 超出时间范围
        }
        dailyCount[serialOf(borrowDate)]++;
    }

    if (dailyCount.empty()) {
        forecast.predictedDemand = 0;
        return forecast;
    }

    // 线性回归预测
    double n = static_cast<double>(dailyCount.size());
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;

    for (const auto& [serial, count] : dailyCount) {
        sumX += serial;
        sumY += count;
        sumXY += serial * count;
        sumX2 += serial * serial;
    }

    double denominator = n * sumX2 - sumX * sumX;
    double slope = 0.0;
    if (std::abs(denominator) > 1e-9) {
        slope = (n * sumXY - sumX * sumY) / denominator;
    }
    double intercept = (sumY - slope * sumX) / n;
    double avg = sumY / n;

    // 预测明天的需求
    long long futureSerial = serialOf(DateUtil::addDays(today, 1));
    double trendValue = intercept + slope * static_cast<double>(futureSerial);
    double combined = 0.4 * avg + 0.6 * trendValue;  // 权重偏向趋势

    forecast.predictedDemand = std::max(0, static_cast<int>(std::round(combined)));
    return forecast;
}

// 预测热门书籍（基于当前借阅次数）
std::vector<std::string> DataAnalyzer::predictPopularBooks(int days) {
    (void)days;  // 未使用参数
    auto popularity = m_borrowService.getPopularBooks(20);
    std::vector<std::pair<std::string, int>> ranking(popularity.begin(), popularity.end());
    std::sort(ranking.begin(), ranking.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;  // 次数相同按ISBN排序
        }
        return lhs.second > rhs.second;  // 按次数降序
        });

    std::vector<std::string> result;
    for (const auto& [isbn, _count] : ranking) {
        result.push_back(isbn);
    }
    return result;
}

// 生成借阅图表数据
ChartData DataAnalyzer::generateBorrowChart(DateRange range) {
    ensureRange(range);
    ChartData chart{};
    auto trend = analyzeBorrowTrend(range);

    for (const auto& [date, count] : trend.dailyCounts) {
        chart.series.emplace_back(DateUtil::toString(date), static_cast<double>(count));
    }
    return chart;
}

// 生成分析报告
ReportData DataAnalyzer::generateAnalyticsReport() {
    ReportData report{};
    report.title = "图书馆运营分析报告";

    std::ostringstream oss;

    // 类别受欢迎程度分析
    auto categoryPopularity = analyzeCategoryPopularity();
    oss << "类别受欢迎程度：\n";
    for (const auto& [category, ratio] : categoryPopularity) {
        oss << "  - " << category << ": " << std::fixed << std::setprecision(2) << (ratio * 100.0) << "%\n";
    }
    report.sections.push_back(oss.str());
    oss.str("");
    oss.clear();

    // 读者分群分析
    auto segments = segmentReaders();
    oss << "读者分群：\n";
    for (const auto& segment : segments) {
        oss << "  - " << segment.name << ": " << segment.readerIds.size() << " 人\n";
    }
    report.sections.push_back(oss.str());
    oss.str("");
    oss.clear();

    // 图书利用率分析
    auto utilization = analyzeBookUtilization();
    oss << "图书利用率：\n";
    oss << "  平均借阅次数：" << std::fixed << std::setprecision(2) << utilization.averageBorrowPerBook << "\n";
    oss << "  高利用图书数量：" << utilization.highUtilizationBooks.size() << "\n";
    oss << "  低利用图书数量：" << utilization.lowUtilizationBooks.size() << "\n";
    report.sections.push_back(oss.str());

    return report;
}

// 收集指定日期范围内的借阅记录
std::vector<BorrowRecord> DataAnalyzer::collectRecordsInRange(DateRange range) {
    ensureRange(range);
    return m_borrowService.getRecordsByDateRange(range.start, range.end);
}

// 计算每个类别的借阅次数
std::map<std::string, int> DataAnalyzer::calculateCategoryBorrowCount() {
    std::map<std::string, int> categoryCount;
    auto records = m_borrowService.getRecordsByDateRange(Date{ 1970, 1, 1 }, DateUtil::today());

    for (const auto& record : records) {
        auto books = m_bookService.searchByISBN(record.getISBN());
        if (!books.empty()) {
            categoryCount[books.front().getCategory()]++;
        }
    }
    return categoryCount;
}

// 计算平均借阅时长
double DataAnalyzer::calculateAverageBorrowDuration(const Reader& reader) {
    auto records = m_borrowService.getReaderBorrowHistory(reader.getReaderId());
    if (records.empty()) {
        return 0.0;
    }

    double totalDays = 0.0;
    int counted = 0;

    // 只统计已归还的记录
    for (const auto& record : records) {
        if (record.isReturned()) {
            totalDays += DateUtil::daysBetween(record.getBorrowDate(), record.getReturnDate().value());
            ++counted;
        }
    }

    if (counted == 0) {
        return 0.0;
    }
    return totalDays / counted;  // 平均借阅天数
}
