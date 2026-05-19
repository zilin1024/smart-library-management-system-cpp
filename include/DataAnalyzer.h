#ifndef DATA_ANALYZER_H
#define DATA_ANALYZER_H

#include "BookService.h"
#include "BorrowService.h"
#include "ReaderService.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

// ---- 辅助结构体 ----------------------------------------------------------

struct DateRange {
    Date start;
    Date end;
};

struct BorrowTrend {
    std::map<Date, int> dailyCounts;
};

struct PeakHours {
    std::vector<int> hourlyCounts;
    int peakHour{ 0 };
};

struct ReaderProfile {
    std::string readerId;
    int totalBorrowed{ 0 };
    std::vector<std::string> favoriteCategories;
    double averageBorrowDuration{ 0.0 };
};

struct ReaderSegment {
    std::string name;
    std::vector<std::string> readerIds;
};

struct BookUtilization {
    double averageBorrowPerBook{ 0.0 };
    std::vector<std::string> highUtilizationBooks;
    std::vector<std::string> lowUtilizationBooks;
};

struct BorrowPrediction {
    std::map<Date, int> dailyPrediction;
};

struct DemandForecast {
    std::string isbn;
    int predictedDemand{ 0 };
};

struct ChartData {
    std::vector<std::pair<std::string, double>> series;
};

struct ReportData {
    std::string title;
    std::vector<std::string> sections;
};

// ---- 数据分析引擎 --------------------------------------------------------

class DataAnalyzer {
public:
    DataAnalyzer(BookService& bookService,
        BorrowService& borrowService,
        ReaderService& readerService);

    // 借阅分析
    BorrowTrend analyzeBorrowTrend(DateRange range);
    PeakHours analyzePeakBorrowHours();
    std::map<std::string, double> analyzeCategoryPopularity();

    // 读者行为分析
    ReaderProfile generateReaderProfile(const std::string& readerId);
    std::vector<ReaderSegment> segmentReaders();
    std::map<std::string, double> calculateReaderEngagement();

    // 图书利用率分析
    BookUtilization analyzeBookUtilization();
    std::vector<std::string> identifyUnderusedBooks(double threshold);
    std::vector<std::string> identifyOverusedBooks(double threshold);

    // 预测分析
    BorrowPrediction predictFutureBorrows(int days = 30);
    DemandForecast forecastBookDemand(const std::string& isbn);
    std::vector<std::string> predictPopularBooks(int days = 30);

    // 可视化数据
    ChartData generateBorrowChart(DateRange range);
    ReportData generateAnalyticsReport();

private:
    BookService& m_bookService;
    BorrowService& m_borrowService;
    ReaderService& m_readerService;

    std::vector<BorrowRecord> collectRecordsInRange(DateRange range);
    std::map<std::string, int> calculateCategoryBorrowCount();
    double calculateAverageBorrowDuration(const Reader& reader);
};

#endif // DATA_ANALYZER_H
