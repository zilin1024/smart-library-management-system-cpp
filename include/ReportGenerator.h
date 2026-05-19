#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include "BookService.h"
#include "BorrowService.h"
#include "DataAnalyzer.h"
#include "FileManager.h"
#include "NotificationCenter.h"
#include "ReaderService.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * @struct ReportSection
 * @brief 报表章节结构，包含标题与内容。
 */
struct ReportSection {
    std::string title;
    std::string content;
};

/**
 * @struct ReportSummary
 * @brief 报表概要统计信息。
 */
struct ReportSummary {
    int totalBooks{ 0 };
    int totalReaders{ 0 };
    int totalBorrowRecords{ 0 };
    double averageBorrowPerBook{ 0.0 };
    int overdueCount{ 0 };
    double totalFineCollected{ 0.0 };
};

/**
 * @class ReportGenerator
 * @brief 高级功能层报表生成器，负责汇总系统关键指标并输出多种格式的报告。
 */
class ReportGenerator {
public:
    ReportGenerator(BookService& bookService,
        ReaderService& readerService,
        BorrowService& borrowService,
        DataAnalyzer& dataAnalyzer,
        FileManager& fileManager,
        NotificationCenter& notificationCenter);

    // === 报表生成 ===
    std::string generateDailyReport(const Date& date);
    std::string generateMonthlyReport(int year, int month);
    std::string generateCustomReport(const std::string& title,
        const std::vector<ReportSection>& sections);

    // === 导出功能 ===
    bool exportReportToFile(const std::string& reportContent, const std::string& filename);
    bool exportReportToHTML(const std::string& reportContent, const std::string& filename);
    bool exportReportToMarkdown(const std::string& reportContent, const std::string& filename);

    // === 概要统计 ===
    ReportSummary summarizeSystemStatus() const;
    std::vector<std::pair<std::string, double>> topBorrowedCategories(int limit = 5);
    std::vector<std::pair<std::string, int>> topActiveReaders(int limit = 5);

    // === 通知与分发 ===
    bool notifyAdministrators(const std::string& reportTitle, const std::string& message);
    bool scheduleReportDelivery(const std::string& reportTitle, const Date& deliveryDate);

private:
    BookService& m_bookService;
    ReaderService& m_readerService;
    BorrowService& m_borrowService;
    DataAnalyzer& m_dataAnalyzer;
    FileManager& m_fileManager;
    NotificationCenter& m_notificationCenter;

    std::vector<std::string> m_reportQueue;

    std::string buildHeader(const std::string& title) const;
    std::string buildFooter() const;
    std::string formatSection(const ReportSection& section) const;
    std::string formatSummary(const ReportSummary& summary) const;

    std::vector<std::pair<std::string, int>> collectTopBorrowersInternal(int limit) const;
};

#endif // REPORT_GENERATOR_H
