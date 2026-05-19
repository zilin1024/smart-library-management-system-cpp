#include "ReportGenerator.h"
#include "DateUtil.h"
#include "Reader.h"
#include "StringUtil.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

// ==== 构造函数 =============================================================

// 初始化报表生成器，注入所需的所有服务依赖
ReportGenerator::ReportGenerator(BookService& bookService,
    ReaderService& readerService,
    BorrowService& borrowService,
    DataAnalyzer& dataAnalyzer,
    FileManager& fileManager,
    NotificationCenter& notificationCenter)
    : m_bookService(bookService),
    m_readerService(readerService),
    m_borrowService(borrowService),
    m_dataAnalyzer(dataAnalyzer),
    m_fileManager(fileManager),
    m_notificationCenter(notificationCenter) {}

// ==== 报表生成 =============================================================

// 生成日报：包含当日借阅统计、系统概况及借阅趋势
std::string ReportGenerator::generateDailyReport(const Date& date) {
    BorrowStatistics dailyStats = m_borrowService.getDailyStatistics(date);
    ReportSummary summary = summarizeSystemStatus();

    std::ostringstream oss;
    oss << buildHeader("每日运营报告 - " + DateUtil::toString(date));
    oss << formatSummary(summary);

    // 构建借阅概览部分
    ReportSection borrowSection{
        "借阅概览",
        "借出数量: " + std::to_string(dailyStats.totalBorrowed) + "\n"
        "归还数量: " + std::to_string(dailyStats.totalReturned) + "\n"
        "逾期数量: " + std::to_string(dailyStats.overdueCount) + "\n"
        "罚金合计: " + std::to_string(static_cast<long double>(dailyStats.totalFines))
    };
    oss << formatSection(borrowSection);

    // 分析当日借阅趋势
    auto trendingThreads = m_dataAnalyzer.analyzeBorrowTrend({ date, date });
    ReportSection trendSection{
        "借阅趋势",
        "当日借阅次数: " + std::to_string(trendingThreads.dailyCounts[date])
    };
    oss << formatSection(trendSection);

    oss << buildFooter();
    return oss.str();
}

// 生成月报：包含月度统计、热门分类占比及活跃读者Top5
std::string ReportGenerator::generateMonthlyReport(int year, int month) {
    BorrowStatistics monthlyStats = m_borrowService.getMonthlyStatistics(year, month);
    ReportSummary summary = summarizeSystemStatus();

    std::ostringstream oss;
    oss << buildHeader("月度运营报告 - " + std::to_string(year) + "-" + (month < 10 ? "0" : "") + std::to_string(month));
    oss << formatSummary(summary);

    // 构建月度统计部分
    ReportSection statsSection{
        "月度借阅统计",
        "借出数量: " + std::to_string(monthlyStats.totalBorrowed) + "\n"
        "归还数量: " + std::to_string(monthlyStats.totalReturned) + "\n"
        "逾期次数: " + std::to_string(monthlyStats.overdueCount) + "\n"
        "罚金合计: " + std::to_string(static_cast<long double>(monthlyStats.totalFines))
    };
    oss << formatSection(statsSection);

    // 获取并格式化热门分类数据
    auto topCategories = topBorrowedCategories(5);
    std::ostringstream catStream;
    for (const auto& [category, ratio] : topCategories) {
        catStream << " - " << category << " : "
            << std::fixed << std::setprecision(2) << ratio * 100.0 << "%\n";
    }
    ReportSection categorySection{ "热门分类占比", catStream.str() };
    oss << formatSection(categorySection);

    // 获取并格式化活跃读者数据
    auto activeReaders = topActiveReaders(5);
    std::ostringstream readerStream;
    for (const auto& [readerId, count] : activeReaders) {
        readerStream << " - " << readerId << " : " << count << " 次借阅\n";
    }
    ReportSection readerSection{ "活跃读者 TOP5", readerStream.str() };
    oss << formatSection(readerSection);

    oss << buildFooter();
    return oss.str();
}

// 生成自定义报表：根据传入的章节列表组装
std::string ReportGenerator::generateCustomReport(const std::string& title,
    const std::vector<ReportSection>& sections) {
    std::ostringstream oss;
    oss << buildHeader(title);

    for (const auto& section : sections) {
        oss << formatSection(section);
    }

    oss << buildFooter();
    return oss.str();
}

// ==== 导出功能 =============================================================

// 将报表内容导出到普通文本文件
bool ReportGenerator::exportReportToFile(const std::string& reportContent, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    ofs << reportContent;
    return true;
}

// 将报表内容导出为简单的 HTML 格式
bool ReportGenerator::exportReportToHTML(const std::string& reportContent, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    ofs << "<html><head><meta charset=\"utf-8\"><title>Library Report</title></head><body><pre>\n";
    ofs << reportContent;
    ofs << "\n</pre></body></html>\n";
    return true;
}

// 将报表内容导出为 Markdown（复用文本导出）
bool ReportGenerator::exportReportToMarkdown(const std::string& reportContent, const std::string& filename) {
    return exportReportToFile(reportContent, filename);
}

// ==== 概要统计 =============================================================

// 汇总系统当前的核心指标状态
ReportSummary ReportGenerator::summarizeSystemStatus() const {
    ReportSummary summary;
    summary.totalBooks = m_bookService.getTotalBookCount();
    summary.totalReaders = m_readerService.getActiveReaderCount();
    // 估算总记录数（通过获取全部热门书籍列表的大小来近似）
    summary.totalBorrowRecords = static_cast<int>(m_borrowService.getPopularBooks(0).size());

    // 分析图书利用率
    auto utilization = m_dataAnalyzer.analyzeBookUtilization();
    summary.averageBorrowPerBook = utilization.averageBorrowPerBook;

    // 统计当前逾期数
    auto overdueRecords = m_borrowService.getOverdueRecords();
    summary.overdueCount = static_cast<int>(overdueRecords.size());

    // 计算最近30天的罚金总额
    double fines = 0.0;
    auto records = m_borrowService.getRecordsByDateRange(DateUtil::addDays(DateUtil::today(), -30), DateUtil::today());
    for (const auto& record : records) {
        fines += record.getFine();
    }
    summary.totalFineCollected = fines;
    return summary;
}

// 获取借阅量最高的分类
std::vector<std::pair<std::string, double>> ReportGenerator::topBorrowedCategories(int limit) {
    auto popularity = m_dataAnalyzer.analyzeCategoryPopularity();
    std::vector<std::pair<std::string, double>> vec(popularity.begin(), popularity.end());
    // 按比例降序排序
    std::sort(vec.begin(), vec.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second > rhs.second;
        });
    if (limit > 0 && static_cast<std::size_t>(limit) < vec.size()) {
        vec.resize(static_cast<std::size_t>(limit));
    }
    return vec;
}

// 获取最活跃的读者（借阅次数最多）
std::vector<std::pair<std::string, int>> ReportGenerator::topActiveReaders(int limit) {
    return collectTopBorrowersInternal(limit);
}

// ==== 通知与分发 ===========================================================

// 通知管理员：尝试查找 ID 以 "ADM" 开头的读者，若无则通知活跃读者
bool ReportGenerator::notifyAdministrators(const std::string& reportTitle, const std::string& message) {
    auto readers = m_readerService.getReadersByBorrowCount(0);
    std::vector<std::string> adminIds;
    for (const auto& reader : readers) {
        if (!reader.isActive()) {
            continue;
        }
        // 约定：ID 以 "ADM" 开头为管理员
        if (StringUtil::startsWith(reader.getReaderId(), "ADM", true)) {
            adminIds.push_back(reader.getReaderId());
        }
    }

    // 如果未找到管理员账号，回退策略：通知 Top3 活跃读者（模拟行为）
    if (adminIds.empty()) {
        for (const auto& pair : collectTopBorrowersInternal(3)) {
            adminIds.push_back(pair.first);
        }
    }

    bool sent = false;
    std::string payload = "[报告通知] " + reportTitle + "\n" + message;
    for (const auto& id : adminIds) {
        sent = m_notificationCenter.sendCustomNotification(id, payload) || sent;
    }
    return sent;
}

// 计划报告投递（模拟放入队列）
bool ReportGenerator::scheduleReportDelivery(const std::string& reportTitle, const Date& deliveryDate) {
    if (!DateUtil::isValid(deliveryDate)) {
        return false;
    }
    std::ostringstream oss;
    oss << reportTitle << " :: " << DateUtil::toString(deliveryDate);
    m_reportQueue.push_back(oss.str());
    return true;
}

// ==== 私有工具 =============================================================

// 构建报告头部
std::string ReportGenerator::buildHeader(const std::string& title) const {
    std::ostringstream oss;
    oss << "==============================\n";
    oss << title << "\n";
    oss << "生成时间： " << DateUtil::toString(DateUtil::today()) << "\n";
    oss << "==============================\n\n";
    return oss.str();
}

// 构建报告尾部
std::string ReportGenerator::buildFooter() const {
    return "\n==============================\n报告结束\n==============================\n";
}

// 格式化单个报告章节
std::string ReportGenerator::formatSection(const ReportSection& section) const {
    std::ostringstream oss;
    oss << ">> " << section.title << "\n";
    oss << section.content << "\n\n";
    return oss.str();
}

// 格式化系统概况汇总
std::string ReportGenerator::formatSummary(const ReportSummary& summary) const {
    std::ostringstream oss;
    oss << "系统概要统计\n";
    oss << " - 总图书数: " << summary.totalBooks << "\n";
    oss << " - 活跃读者数: " << summary.totalReaders << "\n";
    oss << " - 借阅记录数: " << summary.totalBorrowRecords << "\n";
    oss << " - 平均借阅次数/图书: " << std::fixed << std::setprecision(2) << summary.averageBorrowPerBook << "\n";
    oss << " - 当前逾期数量: " << summary.overdueCount << "\n";
    oss << " - 最近30天罚金: " << std::fixed << std::setprecision(2) << summary.totalFineCollected << "\n\n";
    return oss.str();
}

// 内部工具：收集借阅量最高的读者ID和数量
std::vector<std::pair<std::string, int>> ReportGenerator::collectTopBorrowersInternal(int limit) const {
    std::vector<std::pair<std::string, int>> result;
    auto readers = m_readerService.getTopBorrowers(limit);
    for (const auto& reader : readers) {
        result.emplace_back(reader.getReaderId(), reader.getTotalBorrowed());
    }
    return result;
}
