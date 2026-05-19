/*******************************************************
 * 综合示例：图书馆管理系统功能演示（主程序）
 * 说明：
 *   1. 演示顺序按照“基础功能 → 进阶功能 → 工具/支撑功能”展开；
 *   2. 覆盖方案中提到的全部接口；若某接口属于占位演示，会在输出中说明；
 *   3. 代码项目在 C++20 或更新标准下编译。
 *******************************************************/
#include "AIRecommender.h"
#include "AdvancedSearch.h"
#include "Book.h"
#include "BookService.h"
#include "BorrowRecord.h"
#include "BorrowService.h"
#include "CommunityManager.h"
#include "CreditSystem.h"
#include "DataAnalyzer.h"
#include "DateUtil.h"
#include "Encryption.h"
#include "FileManager.h"
#include "InteractiveShell.h"
#include "LibraryManager.h"
#include "Logger.h"
#include "MultiTerminalSync.h"
#include "NotificationCenter.h"
#include "Reader.h"
#include "ReaderService.h"
#include "ReportGenerator.h"
#include "ReservationSystem.h"
#include "SearchService.h"
#include "SecurityManager.h"
#include "StringUtil.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using namespace DateUtil;

/* -------------------- 辅助结构体 -------------------- */
struct ReservationDemoContext {
    std::string isbn;
    std::map<std::string, std::string> reservationIdByReader;
};
struct CommunityDemoContext {
    std::string threadId;
    std::string postId;
    std::string commentId;
};
/* -------------------- 辅助输出函数 -------------------- */
void printBookList(const std::vector<Book>& books, const std::string& title) {
    std::cout << "\n==== " << title << "（共 " << books.size() << " 本）====\n";
    for (const auto& book : books) {
        std::cout << "ISBN：" << book.getISBN()
            << " | 书名：" << book.getTitle()
            << " | 作者：" << book.getAuthor()
            << " | 库存：" << book.getStock()
            << " | 类别：" << book.getCategory() << '\n';
    }
}
void printReaderList(const std::vector<Reader>& readers, const std::string& title) {
    std::cout << "\n==== " << title << "（共 " << readers.size() << " 人）====\n";
    for (const auto& reader : readers) {
        std::cout << "读者ID：" << reader.getReaderId()
            << " | 姓名：" << reader.getName()
            << " | 当前借阅：" << reader.getCurrentBorrowCount()
            << " | 累计借阅：" << reader.getTotalBorrowed()
            << " | 状态：" << (reader.isActive() ? "已激活" : "未激活")
            << " | 是否停借：" << (reader.isSuspended() ? "是" : "否") << '\n';
    }
}
void printBorrowRecords(const std::vector<BorrowRecord>& records, const std::string& title) {
    std::cout << "\n==== " << title << "（共 " << records.size() << " 条）====\n";
    for (const auto& record : records) {
        std::cout << "记录ID：" << record.getRecordId()
            << " | 读者ID：" << record.getReaderId()
            << " | ISBN：" << record.getISBN()
            << " | 借出日期：" << DateUtil::toString(record.getBorrowDate())
            << " | 应还日期：" << DateUtil::toString(record.getDueDate());
        if (record.getReturnDate().has_value()) {
            std::cout << " | 实还日期：" << DateUtil::toString(record.getReturnDate().value());
        }
        else {
            std::cout << " | 尚未归还";
        }
        std::cout << " | 续借次数：" << record.getRenewCount()
            << " | 当前罚金：" << std::fixed << std::setprecision(2) << record.getFine()
            << '\n';
    }
}
void printCategoryDistribution(const std::map<std::string, int>& data, const std::string& title) {
    std::cout << "\n==== " << title << " ====\n";
    for (const auto& item : data) {
        std::cout << "类别：" << item.first << " -> 数量：" << item.second << '\n';
    }
}
void printYearDistribution(const std::map<int, int>& data, const std::string& title) {
    std::cout << "\n==== " << title << " ====\n";
    for (const auto& item : data) {
        std::cout << "年份：" << item.first << " -> 数量：" << item.second << '\n';
    }
}
void printStringVector(const std::vector<std::string>& data, const std::string& title) {
    std::cout << "\n==== " << title << " ====\n";
    for (const auto& item : data) {
        std::cout << "- " << item << '\n';
    }
}
void printBorrowStatistics(const BorrowStatistics& stats, const std::string& title) {
    std::cout << "\n==== " << title << " ====\n";
    std::cout << "借出总数：" << stats.totalBorrowed
        << " | 归还总数：" << stats.totalReturned
        << " | 逾期次数：" << stats.overdueCount
        << " | 罚金合计：" << std::fixed << std::setprecision(2) << stats.totalFines << '\n';
}
/* -------------------- 各模块演示函数声明 -------------------- */
void demoBookServiceExtended(BookService& bookService, LibraryManager& libraryManager);
void demoReaderServiceExtended(ReaderService& readerService);
void demoBorrowServiceExtended(BorrowService& borrowService,
    BookService& bookService,
    ReaderService& readerService,
    LibraryManager& libraryManager);
void demoSearchAndAdvanced(SearchService& searchService,
    AdvancedSearch& advancedSearch,
    BookService& bookService,
    ReaderService& readerService,
    BorrowService& borrowService);
void demoAIRecommender(AIRecommender& aiRecommender, BookService& bookService);
void demoDataAnalyzer(DataAnalyzer& dataAnalyzer);
ReservationDemoContext demoReservationSystem(ReservationSystem& reservationSystem,
    BookService& bookService,
    ReaderService& readerService,
    BorrowService& borrowService);
void demoNotificationCenter(NotificationCenter& notificationCenter,
    BorrowService& borrowService,
    ReaderService& readerService,
    LibraryManager& libraryManager,
    const ReservationDemoContext& context);
void demoCreditSystem(CreditSystem& creditSystem, ReaderService& readerService);
CommunityDemoContext demoCommunityManager(CommunityManager& communityManager,
    NotificationCenter& notificationCenter);
void demoSecurityManager(SecurityManager& securityManager);
void demoMultiTerminalSync(MultiTerminalSync& multiSync);
void demoReportGenerator(ReportGenerator& reportGenerator, BorrowService& borrowService);
void demoFileManager(FileManager& fileManager, LibraryManager& libraryManager, BookService& bookService);
void demoEncryptionAndLogger();

/* -------------------- 各模块演示函数实现 -------------------- */
void demoBookServiceExtended(BookService& bookService, LibraryManager& libraryManager) {
    std::cout << "\n==================== 图书服务扩展演示 ====================\n";

    // 1. 批量新增图书
    std::vector<Book> newBooks = {
        Book("978-0000000001", "并发编程实践", "张衡", "清华大学出版社", 2024, "并发编程", 2),
        Book("978-0000000002", "数据湖架构设计", "李倩", "电子工业出版社", 2023, "大数据", 0)
    };
    bookService.batchAddBooks(newBooks);
    printBookList(bookService.searchByKeyword(""), "批量新增图书后列表");

    // 2. 修改图书信息（书名 / 出版社 / 库存 / 标签）
    BookUpdate updateInfo;
    updateInfo.title = "并发编程实践（第二版）";
    updateInfo.publisher = "机械工业出版社";
    updateInfo.stock = 5;
    updateInfo.tags = std::vector<std::string>{ "多线程", "C++20" };
    bookService.updateBookInfo("978-0000000001", updateInfo);
    std::cout << "已将 ISBN 978-0000000001 更新为第二版。\n";

    // 3. 库存管理：增加/减少/直接设置
    bookService.increaseStock("978-0131103627", 2);
    bookService.decreaseStock("978-0131103627", 1);
    bookService.setStock("978-0000000002", 3);
    bookService.updateStock("978-7302598980", 7);
    std::cout << "已完成多组库存调整操作。\n";

    // 4. 统计信息输出
    printCategoryDistribution(bookService.getCategoryDistribution(), "按类别统计图书数量");
    printYearDistribution(bookService.getYearDistribution(), "按年份统计图书数量");
    auto publisherDist = bookService.getPublisherDistribution();
    std::cout << "\n==== 出版社统计 ====\n";
    for (const auto& item : publisherDist) {
        std::cout << "出版社：" << item.first << " -> 数量：" << item.second << '\n';
    }
    printBookList(bookService.getLowStockBooks(2), "低库存（≤2）图书");
    // 4+. 查询演示：按标题 / 作者 / 类别 / 多条件
    printBookList(bookService.searchByTitle("C++"), "按标题包含“C++”的图书");
    printBookList(bookService.searchByAuthor("Scott"), "按作者包含“Scott”的图书");
    printBookList(bookService.searchByCategory("Programming"), "类别为 Programming 的图书");

    SearchCriteria combined;
    combined.author = "Scott";
    combined.category = "Programming";
    combined.inStockOnly = true;
    auto combinedResult = bookService.searchByMultipleCriteria(combined);
    printBookList(combinedResult, "多条件（作者=Scott & 类别=Programming & 在库）查询结果");


    // 5. 为新书打分，演示 Book::addRating + 统计
    libraryManager.updateBook("978-0000000001", [](Book& book) {
        book.addRating(4.5);
        book.addRating(4.8);
        });
    std::cout << "已为《并发编程实践（第二版）》添加评分信息。\n";

    // 6. 删除图书（按书名 + 按 ISBN）
    bookService.deleteBookByTitle("数据湖架构设计");
    bookService.deleteBook("978-0000000001");
    std::cout << "已删除演示用的新增图书，恢复原始列表。\n";
}

void demoReaderServiceExtended(ReaderService& readerService) {
    std::cout << "\n==================== 读者服务扩展演示 ====================\n";

    // 1. 新增临时读者并修改信息
    Reader tempReader("R900", "测试读者A", { "13900009000", "temp@example.com", "演示城市" }, 3, 28);
    readerService.registerReader(tempReader);
    std::cout << "已注册临时读者 R900。\n";

    ReaderUpdate update;
    update.name = "测试读者A-更新姓名";
    update.maxBorrow = 6;
    update.age = 30;
    ContactInfo newContact{ "13900009001", "temp_new@example.com", "演示城市新区" };
    update.contact = newContact;
    readerService.updateReaderInfo("R900", update);
    std::cout << "已完成临时读者的信息更新。\n";

    // 2. 调整正式读者信息
    ContactInfo aliceContact{ "13800000011", "alice_new@example.com", "北京市朝阳区" };
    readerService.updateContactInfo("R001", aliceContact);
    readerService.updateMaxBooks("R001", 7);
    std::cout << "已为 R001 更新联系方式与最大借书量。\n";

    // 3. 演示停借 / 解除停借
    readerService.deactivateReader("R003");
    readerService.activateReader("R003");
    std::cout << "已对 R003 完成停借与重新激活操作。\n";

    // 4. 按姓名模糊搜索、按借阅量筛选
    printReaderList(readerService.searchReadersByName("读者"), "姓名包含“读者”的读者列表");
    printReaderList(readerService.getReadersByBorrowCount(1), "借阅次数不少于 1 次的读者");

    // 5. 统计信息
    auto ageDist = readerService.getReaderAgeDistribution();
    std::cout << "\n==== 读者年龄分布 ====\n";
    for (const auto& item : ageDist) {
        std::cout << "年龄：" << item.first << " -> 人数：" << item.second << '\n';
    }
    printReaderList(readerService.getTopBorrowers(3), "借阅排行榜 TOP3");

    // 6. 删除临时读者
    readerService.permanentlyDeleteReader("R900");
    std::cout << "已删除临时读者 R900。\n";
}

void demoBorrowServiceExtended(BorrowService& borrowService,
    BookService& bookService,
    ReaderService& readerService,
    LibraryManager& libraryManager) {
    std::cout << "\n==================== 借阅服务扩展演示 ====================\n";

    borrowService.setFinePerDay(1.5);
    borrowService.setDefaultBorrowDays(21);
    borrowService.setMaxRenewals(3);
    std::cout << "已调整罚金、借期和最大续借次数参数。\n";

    // 1. 检查借阅资格 → 借书
    bool canBorrow = borrowService.checkBorrowEligibility("R003", "978-0596007126");
    std::cout << "R003 借阅《Head First Design Patterns》的资格：" << (canBorrow ? "允许" : "不允许") << '\n';
    if (canBorrow) {
        BorrowResult borrowResult = borrowService.borrowBook("R003", "978-0596007126");
        std::cout << "借阅结果：" << borrowResult.message << '\n';
    }

    // 2. 准备一批演示用图书并借出 → 续借
    Book tempBook("978-5555555555", "元编程指南", "王凯", "人民邮电出版社", 2022, "编程范式", 1);
    bookService.addBook(tempBook);
    BorrowResult tempBorrow = borrowService.borrowBook("R002", "978-5555555555");
    std::cout << "R002 借阅《元编程指南》：" << tempBorrow.message << '\n';
    borrowService.renewBook("R002", "978-5555555555");
    std::cout << "已为 R002 续借《元编程指南》一次。\n";

    // 3. 预约（借阅层占位接口）演示
    bool reserveResult = borrowService.reserveBook("R001", "978-5555555555");
    std::cout << "调用 BorrowService::reserveBook 的结果：" << (reserveResult ? "已预订" : "预订失败（仅演示）") << '\n';

    // 4. 查看借阅记录（读者 / 图书）
    printBorrowRecords(borrowService.getReaderBorrowHistory("R002"), "R002 的借阅历史");
    printBorrowRecords(borrowService.getBookBorrowHistory("978-5555555555"), "《元编程指南》的借阅历史");

    // 5. 逾期记录演示：强制修改应还日期为过去
    auto history = borrowService.getReaderBorrowHistory("R002");
    if (!history.empty()) {
        const std::string overdueId = history.front().getRecordId();
        libraryManager.updateBorrowRecord(overdueId, [](BorrowRecord& record) {
            record.setDueDate(DateUtil::addDays(DateUtil::today(), -3));
            });
        std::cout << "已将记录 " << overdueId << " 的应还日期改为 3 天前，用于演示逾期。\n";
    }

    auto overdueRecords = borrowService.getOverdueRecords();
    printBorrowRecords(overdueRecords, "逾期记录列表");
    if (!overdueRecords.empty()) {
        double fine = borrowService.calculateOverdueFine(overdueRecords.front());
        std::cout << "首条逾期记录的罚金计算结果：" << fine << " 元。\n";
    }

    // 6. 当前在借列表 / 时间范围查询
    printBorrowRecords(borrowService.getCurrentBorrows("R002"), "R002 当前在借列表");
    DateRange recentRange{ DateUtil::addDays(DateUtil::today(), -7), DateUtil::today() };
    printBorrowRecords(borrowService.getRecordsByDateRange(recentRange.start, recentRange.end),
        "最近 7 天的借阅记录");
    // 8. 还书演示
    BorrowResult quickBorrow = borrowService.borrowBook("R001", "978-0131103627");
    if (quickBorrow.status == BorrowStatus::Success) {
        std::cout << "为 R001 临时借出《The C Programming Language》用于还书示例。\n";
        ReturnResult quickReturn = borrowService.returnBook("R001", "978-0131103627");
        std::cout << "还书结果：" << quickReturn.message
            << " | 罚金：" << std::fixed << std::setprecision(2) << quickReturn.fine << '\n';
    }
    else {
        std::cout << "示例还书未执行，原因：" << quickBorrow.message << '\n';
    }


    // 7. 当日 / 当月统计
    printBorrowStatistics(borrowService.getDailyStatistics(DateUtil::today()), "今日借阅统计");
    printBorrowStatistics(borrowService.getMonthlyStatistics(DateUtil::today().year, DateUtil::today().month),
        "本月借阅统计");

    std::cout << "借阅服务扩展示例完毕。\n";
}

void demoSearchAndAdvanced(SearchService& searchService,
    AdvancedSearch& advancedSearch,
    BookService& bookService,
    ReaderService& readerService,
    BorrowService& borrowService) {
    std::cout << "\n==================== 搜索与高级搜索演示 ====================\n";

    // 1. 复合条件搜索
    SearchCriteria criteria;
    criteria.title = "C++";
    criteria.yearRange = std::make_pair(2000, 2025);
    criteria.inStockOnly = true;
    auto multiResult = bookService.searchByMultipleCriteria(criteria);
    printBookList(multiResult, "组合条件：书名含 C++ 且在库的图书");

    // 2. 关键词查询与语义 / 模糊 / 同义词
    auto keywordBooks = advancedSearch.searchByKeyword("现代");
    printBookList(keywordBooks, "高级搜索：关键词“现代”");
    auto phraseBooks = advancedSearch.searchByPhrase("Design Patterns");
    printBookList(phraseBooks, "高级搜索：短语“Design Patterns”");
    auto semanticBooks = advancedSearch.semanticSearch("并行编程");
    printBookList(semanticBooks, "高级搜索：语义查询“并行编程”");
    auto fuzzyBooks = advancedSearch.fuzzySearch("Programing", 2);
    printBookList(fuzzyBooks, "高级搜索：模糊查询“Programing”");
    auto synonymBooks = advancedSearch.synonymSearch("算法");
    printBookList(synonymBooks, "高级搜索：同义词查询“算法”");

    // 3. 多字段搜索 / 高级筛选
    SearchQuery multiQuery;
    multiQuery.keyword = "C++";
    multiQuery.author = "Scott";
    auto multiFieldBooks = advancedSearch.multiFieldSearch(multiQuery);
    printBookList(multiFieldBooks, "多字段查询：作者含 Scott + 关键词 C++");

    FilterCriteria filters;
    filters.categories = std::set<std::string>{ "Programming", "编程范式" };
    filters.stockRange = std::make_pair(1, 10);
    auto filterBooks = advancedSearch.advancedFilterSearch(filters);
    printBookList(filterBooks, "高级过滤：类别属于编程相关且库存 1-10");

    // 4. 搜索行为分析与建议
    advancedSearch.saveSearchHistory("R001", "C++ Primer");
    advancedSearch.saveSearchHistory("R001", "并发");
    advancedSearch.saveSearchHistory("R002", "算法导论");
    auto suggestions = advancedSearch.getSearchSuggestions("C++");
    printStringVector(suggestions, "标题补全建议（前缀 C++）");
    auto hotKeywords = advancedSearch.getPopularSearches(5);
    printStringVector(hotKeywords, "热门搜索词 TOP5");

    // 5. 与借阅数据相关的搜索辅助
    auto readersByIsbn = searchService.searchReadersByBorrowedISBN("978-0262033848");
    printReaderList(readersByIsbn, "曾借阅《算法导论》的读者");
    DateRange activityRange{ DateUtil::addDays(DateUtil::today(), -1), DateUtil::today() };
    auto activeReaders = searchService.searchReadersByActivity(activityRange.start, activityRange.end);
    printReaderList(activeReaders, "最近 1 天内活跃借阅的读者");
    auto borrowedTogether = searchService.getBooksBorrowedTogether("978-0262033848", 5);
    printBookList(borrowedTogether, "借阅了《算法导论》的人也常借的图书");
    auto frequentIsbns = searchService.getFrequentlyBorrowedISBNs(3);
    printStringVector(frequentIsbns, "系统内借阅频次最高的 ISBN（示例）");
    auto titleSuggestion = searchService.getBookTitleSuggestions("Head", 5);
    printStringVector(titleSuggestion, "标题自动补全建议（Head）");
    auto authorSuggestion = searchService.getAuthorSuggestions("Sc", 5);
    printStringVector(authorSuggestion, "作者自动补全建议（Sc）");

    std::cout << "搜索与高级搜索演示完毕。\n";
}

void demoAIRecommender(AIRecommender& aiRecommender, BookService& bookService) {
    std::cout << "\n==================== 智能推荐系统演示 ====================\n";

    auto personalized = aiRecommender.recommendForReader("R001", 5);
    printBookList(personalized, "个性化推荐：R001");
    auto historyBased = aiRecommender.recommendBasedOnHistory("R002");
    printBookList(historyBased, "基于历史记录的推荐：R002");
    auto similarBooks = aiRecommender.recommendSimilarBooks("978-0262033848");
    printBookList(similarBooks, "与《算法导论》相似的推荐");

    printBookList(aiRecommender.getTopBorrowedBooks(30, 3), "近 30 天借阅最热的图书（Top3）");
    printBookList(aiRecommender.getTopRatedBooks(3), "评分最高图书（示例 Top3）");
    printBookList(aiRecommender.getNewArrivals(14, 3), "近两周新上架图书（示例）");
    printBookList(aiRecommender.collaborativeFiltering("R001"), "协同过滤推荐：R001");
    printBookList(aiRecommender.getReadersAlsoBorrowed("978-7302598980"), "借过 C++ Primer Plus 的读者还常借");

    aiRecommender.trainRecommendationModel();
    std::cout << "已调用模型训练（占位实现）。\n";
    std::cout << "模型准确率估计值：" << aiRecommender.evaluateRecommendationAccuracy() << '\n';
    aiRecommender.updateReaderPreferences("R003");
    std::cout << "已更新 R003 的偏好画像（示例）。\n";
}

void demoDataAnalyzer(DataAnalyzer& dataAnalyzer) {
    std::cout << "\n==================== 数据分析引擎演示 ====================\n";

    DateRange range{ DateUtil::addDays(DateUtil::today(), -30), DateUtil::today() };
    auto trend = dataAnalyzer.analyzeBorrowTrend(range);
    std::cout << "近 30 天借阅趋势（按日）：\n";
    for (const auto& item : trend.dailyCounts) {
        std::cout << "日期 " << DateUtil::toString(item.first) << " -> 借出 " << item.second << " 本\n";
    }

    auto peak = dataAnalyzer.analyzePeakBorrowHours();
    std::cout << "预计最繁忙借阅时段为：" << peak.peakHour << " 点附近（示例结果）。\n";

    auto popularity = dataAnalyzer.analyzeCategoryPopularity();
    std::cout << "类别受欢迎度（占比）：\n";
    for (const auto& item : popularity) {
        std::cout << item.first << " -> " << std::fixed << std::setprecision(2) << item.second * 100 << "%\n";
    }

    auto profile = dataAnalyzer.generateReaderProfile("R001");
    std::cout << "R001 画像：累计借阅 " << profile.totalBorrowed
        << "，偏好类别：";
    for (const auto& cat : profile.favoriteCategories) std::cout << cat << ' ';
    std::cout << "，平均借阅时长：" << profile.averageBorrowDuration << " 天。\n";

    auto segments = dataAnalyzer.segmentReaders();
    std::cout << "读者分群结果：\n";
    for (const auto& seg : segments) {
        std::cout << "- " << seg.name << "：";
        for (const auto& id : seg.readerIds) std::cout << id << ' ';
        std::cout << '\n';
    }

    auto engagement = dataAnalyzer.calculateReaderEngagement();
    std::cout << "读者活跃度评分（示例）：\n";
    for (const auto& item : engagement) {
        std::cout << "读者 " << item.first << " -> 活跃度 " << item.second << '\n';
    }

    auto utilization = dataAnalyzer.analyzeBookUtilization();
    std::cout << "平均每本图书借阅次数：" << utilization.averageBorrowPerBook << '\n';
    printStringVector(utilization.highUtilizationBooks, "高利用率图书 ISBN 列表");
    printStringVector(utilization.lowUtilizationBooks, "低利用率图书 ISBN 列表");

    printStringVector(dataAnalyzer.identifyUnderusedBooks(1.0), "被判定为利用不足的图书");
    printStringVector(dataAnalyzer.identifyOverusedBooks(3.0), "借阅过于频繁的图书");

    auto prediction = dataAnalyzer.predictFutureBorrows(7);
    std::cout << "未来 7 天借阅量预测：\n";
    for (const auto& item : prediction.dailyPrediction) {
        std::cout << DateUtil::toString(item.first) << " -> 预测借出 " << item.second << " 本\n";
    }

    auto demand = dataAnalyzer.forecastBookDemand("978-7302598980");
    std::cout << "对《C++ Primer Plus》的需求预测值：" << demand.predictedDemand << '\n';
    printStringVector(dataAnalyzer.predictPopularBooks(15), "未来热门图书预测（ISBN 列表）");

    auto chartData = dataAnalyzer.generateBorrowChart(range);
    std::cout << "借阅走势图表数据点数：" << chartData.series.size() << '\n';

    auto analyticsReport = dataAnalyzer.generateAnalyticsReport();
    std::cout << "分析报告标题：" << analyticsReport.title << "\n内容概要：\n";
    for (const auto& section : analyticsReport.sections) {
        std::cout << section << '\n';
    }
}

ReservationDemoContext demoReservationSystem(ReservationSystem& reservationSystem,
    BookService& bookService,
    ReaderService& readerService,
    BorrowService& borrowService) {
    std::cout << "\n==================== 预约系统演示 ====================\n";

    ReservationDemoContext ctx;

    // 准备一本库存为 0 的图书，用于触发预约
    Book reservedBook("978-7777777777", "零库存测试图书", "赵云", "高校出版社", 2025, "测试用书", 0);
    bookService.addBook(reservedBook);
    ctx.isbn = reservedBook.getISBN();

    // 三位读者依次预约，排队形成队列
    auto result1 = reservationSystem.reserveBook("R001", ctx.isbn);
    std::cout << "R001 预约结果：" << result1.message << '\n';
    ctx.reservationIdByReader["R001"] = result1.reservationId;

    auto result2 = reservationSystem.reserveBook("R002", ctx.isbn);
    std::cout << "R002 预约结果：" << result2.message << '\n';
    ctx.reservationIdByReader["R002"] = result2.reservationId;

    auto result3 = reservationSystem.reserveBook("R003", ctx.isbn);
    std::cout << "R003 预约结果：" << result3.message << '\n';
    ctx.reservationIdByReader["R003"] = result3.reservationId;

    // 设置优先级 / 修改预约信息 / 调整排队位置
    reservationSystem.setReservationPriority("R003", ctx.isbn, PriorityLevel::High);
    ReservationUpdate update;
    update.newPickupDate = DateUtil::addDays(DateUtil::today(), 3);
    update.newPriority = PriorityLevel::Critical;
    reservationSystem.modifyReservation(result3.reservationId, update);
    reservationSystem.bumpInQueue(result3.reservationId, 0);

    // 查看队列 / 统计信息
    auto queue = reservationSystem.getReservationQueue(ctx.isbn);
    std::cout << "当前预约队列：";
    for (const auto& readerId : queue) std::cout << readerId << ' ';
    std::cout << '\n';
    std::cout << "R002 在队列中的位置：" << reservationSystem.getQueuePosition("R002", ctx.isbn) << '\n';

    // 模拟图书到库 → 自动通知 / 过期处理
    bookService.increaseStock(ctx.isbn, 2); // 增加库存后可以发放预约
    reservationSystem.processAvailableReservations();
    reservationSystem.notifyNextInQueue(ctx.isbn);
    reservationSystem.expireOldReservations();

    // 统计结果
    auto stats = reservationSystem.getReservationStatistics();
    std::cout << "预约统计：总计 " << stats.totalReservations
        << "，有效 " << stats.activeReservations
        << "，已完成 " << stats.fulfilledReservations
        << "，过期 " << stats.expiredReservations << '\n';
    auto hotBooks = reservationSystem.getMostReservedBooks(5);
    std::cout << "最受欢迎预约图书（Top5）：\n";
    for (const auto& item : hotBooks) {
        std::cout << "ISBN：" << item.first << " -> 排队人数：" << item.second << '\n';
    }
    std::cout << "预约履约率：" << std::fixed << std::setprecision(2)
        << reservationSystem.getReservationFulfillmentRate() * 100 << "%\n";

    // 演示取消与批量预约 / 转移预约
    reservationSystem.cancelReservation("R002", ctx.isbn);
    reservationSystem.createGroupReservation({ "R001", "R003" }, ctx.isbn); // 可能部分失败，但用于演示接口
    reservationSystem.transferReservation("R001", "R002", ctx.isbn);

    std::cout << "预约系统演示完毕。\n";
    return ctx;
}

void demoNotificationCenter(NotificationCenter& notificationCenter,
    BorrowService& borrowService,
    ReaderService& readerService,
    LibraryManager& libraryManager,
    const ReservationDemoContext& context) {
    std::cout << "\n==================== 通知中心演示 ====================\n";

    // 1. 借阅提醒：选取一条借阅记录，让其距离到期 2 天
    auto history = borrowService.getReaderBorrowHistory("R002");
    if (!history.empty()) {
        const std::string remindId = history.front().getRecordId();
        libraryManager.updateBorrowRecord(remindId, [](BorrowRecord& record) {
            record.setDueDate(DateUtil::addDays(DateUtil::today(), 2));
            });
        notificationCenter.sendDueDateReminder("R002");
        std::cout << "已发送到期提醒给 R002。\n";
    }

    // 2. 逾期提醒：选取另一条记录改为过期
    history = borrowService.getReaderBorrowHistory("R003");
    if (!history.empty()) {
        const std::string overdueId = history.front().getRecordId();
        libraryManager.updateBorrowRecord(overdueId, [](BorrowRecord& record) {
            record.setDueDate(DateUtil::addDays(DateUtil::today(), -1));
            });
        notificationCenter.sendOverdueAlert("R003");
        notificationCenter.sendRenewalReminder("R003");
        std::cout << "已向 R003 发送逾期提示与续借提醒。\n";
    }

    // 3. 预约相关通知
    for (const auto& item : context.reservationIdByReader) {
        notificationCenter.sendReservationAvailable(item.first, context.isbn);
        notificationCenter.sendReservationExpiring(item.first, context.isbn);
    }

    // 4. 新书、分类、活动、推广、批量通知
    notificationCenter.sendNewBookNotification("R001", { "978-7302598980", "978-7121360985" });
    notificationCenter.sendCategoryUpdate("Programming", { "978-7302598980" });
    LibraryEvent event{ "EVT-001", "跨年读书夜", DateUtil::addDays(DateUtil::today(), 5), "邀您共度阅读之夜。" };
    notificationCenter.sendLibraryEventNotification(event);
    Promotion promotion{ "PRO-001", "寒假限免专区", "部分电子书限时免费。", DateUtil::today(), DateUtil::addDays(DateUtil::today(), 10) };
    notificationCenter.sendPromotionNotification(promotion);

    notificationCenter.sendCustomNotification("R002", "您有新的积分可领取，请查看信用中心。");
    notificationCenter.sendBulkNotification({ "R001", "R002", "R003" }, "图书馆春节期间开放时间有所调整，请关注官网公告。");

    // 5. 查看 / 标记未读通知
    auto unreadR001 = notificationCenter.getUnreadNotifications("R001");
    std::cout << "R001 未读通知数量：" << unreadR001.size() << '\n';
    if (!unreadR001.empty()) {
        notificationCenter.markAsRead(unreadR001.front().id);
        std::cout << "已将 R001 的首条通知标记为已读。\n";
    }

    auto stats = notificationCenter.getNotificationStatistics();
    std::cout << "通知统计：发送 " << stats.sentCount
        << " 条，送达 " << stats.deliveredCount
        << " 条，已读 " << stats.readCount << " 条。\n";
}

void demoCreditSystem(CreditSystem& creditSystem, ReaderService& readerService) {
    std::cout << "\n==================== 信用体系演示 ====================\n";

    // 1. 信用分操作
    creditSystem.addCreditPoints("R001", 120, "积极借阅奖励");
    creditSystem.deductCreditPoints("R001", 40, "归还稍有延迟");
    std::cout << "R001 当前信用分：" << creditSystem.calculateCreditScore("R001")
        << "，信用等级：" << static_cast<int>(creditSystem.getCreditLevel("R001")) << '\n';

    // 2. 权限信息
    std::cout << "R001 最大可借图书数：" << creditSystem.getMaxBooksAllowed("R001") << '\n';
    std::cout << "R001 最大借阅天数：" << creditSystem.getBorrowDaysAllowed("R001") << '\n';
    std::cout << "R001 罚金折扣率：" << creditSystem.getFineDiscountRate("R001") * 100 << "%\n";

    // 3. 激励机制
    auto rewards = creditSystem.getAvailableRewards("R001");
    if (!rewards.empty()) {
        std::cout << "可兑换奖励如下：\n";
        for (const auto& reward : rewards) {
            std::cout << "- " << reward.id << "：" << reward.name
                << "（需 " << reward.requiredPoints << " 分）\n";
        }
        creditSystem.claimReward("R001", rewards.front().id);
        std::cout << "已兑换奖励：" << rewards.front().name << '\n';
    }
    creditSystem.grantRewardForGoodBehavior("R001");
    std::cout << "已为 R001 发放“良好行为”奖励积分。\n";

    // 4. 违规处理 / 恢复
    creditSystem.issueWarning("R001", "多次逾期归还");
    creditSystem.suspendAccount("R001", 3);
    std::cout << "R001 已被停借 3 天，信用等级重新计算值：" << static_cast<int>(creditSystem.getCreditLevel("R001")) << '\n';
    creditSystem.restoreAccount("R001");
    creditSystem.resetCreditScore("R001", 600);
    std::cout << "R001 已恢复正常状态，信用分重置为 600。\n";
}

CommunityDemoContext demoCommunityManager(CommunityManager& communityManager,
    NotificationCenter& notificationCenter) {
    std::cout << "\n==================== 社区与讨论区演示 ====================\n";

    CommunityDemoContext ctx;

    auto threadIdOpt = communityManager.createThread("978-7121360985",
        "Effective Modern C++ 研讨帖",
        "R002");
    if (!threadIdOpt.has_value()) {
        std::cout << "无法创建讨论串，跳过社区演示。\n";
        return ctx;
    }
    ctx.threadId = threadIdOpt.value();
    std::cout << "已创建讨论串：" << ctx.threadId << '\n';

    auto postIdOpt = communityManager.addPost(ctx.threadId, "R002",
        "第1章对现代 C++ 关键字解释得很透彻，推荐大家认真阅读。");
    if (postIdOpt.has_value()) {
        ctx.postId = postIdOpt.value();
        std::cout << "新增帖子：" << ctx.postId << '\n';
    }

    auto commentIdOpt = communityManager.addComment(ctx.postId, "R001",
        "非常赞同！尤其是 lambda 表达式讲解部分。");
    if (commentIdOpt.has_value()) {
        ctx.commentId = commentIdOpt.value();
        std::cout << "新增评论：" << ctx.commentId << '\n';
    }

    communityManager.likePost(ctx.postId, "R003");
    communityManager.unlikePost(ctx.postId, "R003");

    // 举报并处理
    auto reportIdOpt = communityManager.reportContent(ctx.commentId, "R003", "疑似广告（示例）");
    if (reportIdOpt.has_value()) {
        communityManager.resolveReport(reportIdOpt.value(), false);
        std::cout << "已处理举报：" << reportIdOpt.value() << '\n';
    }

    // 统计与推荐
    auto stats = communityManager.getCommunityStats();
    std::cout << "社区统计：讨论串 " << stats.threadCount
        << " 条，帖子 " << stats.postCount
        << " 条，评论 " << stats.commentCount
        << " 条，活跃读者 " << stats.activeReaderCount << " 人。\n";

    auto hotThreads = communityManager.getTrendingThreads(3);
    std::cout << "热门讨论串示例：\n";
    for (const auto& thread : hotThreads) {
        std::cout << "- " << thread.title << "（创建者：" << thread.creatorId << "）\n";
    }

    auto hotPosts = communityManager.getPopularPosts(3);
    std::cout << "热门帖子示例：\n";
    for (const auto& post : hotPosts) {
        std::cout << "- 帖子ID：" << post.postId << " | 点赞：" << post.likeCount << '\n';
    }

    // 邀请与通知
    communityManager.inviteReaderToThread("R003", ctx.threadId);
    communityManager.notifyThreadUpdate(ctx.threadId, "讨论串有新的精华帖，请及时查看。");

    // 关闭讨论串
    communityManager.closeThread(ctx.threadId);
    std::cout << "讨论串 " << ctx.threadId << " 已关闭。\n";

    return ctx;
}

void demoSecurityManager(SecurityManager& securityManager) {
    std::cout << "\n==================== 安全管理演示 ====================\n";

    // 新建会话并通过 MFA
    std::string sessionId = securityManager.createSession("R001");
    securityManager.markSessionMfa(sessionId, true);
    std::cout << "已为 R001 创建会话并通过多因素认证。\n";

    // 添加策略并检查访问
    AccessPolicy policy;
    policy.policyId = "POL-ADMIN";
    policy.allowedRoles = { "ADMIN" };
    policy.restrictedResources = { "报表中心" };
    policy.requireMfa = true;
    policy.effectiveFrom = DateUtil::today();
    policy.effectiveTo = DateUtil::addDays(DateUtil::today(), 30);
    securityManager.addPolicy(policy);

    bool access = securityManager.checkAccess("R001", "报表中心");
    std::cout << "检查 R001 访问“报表中心”的结果：" << (access ? "允许" : "拒绝") << '\n';

    // 策略开关操作
    AccessPolicy defaultPolicy = policy;
    defaultPolicy.policyId = "DEFAULT-POLICY";
    securityManager.setDefaultPolicy(defaultPolicy);
    securityManager.removePolicy("POL-ADMIN");

    // 审计日志查询
    Date start = DateUtil::addDays(DateUtil::today(), -1);
    Date end = DateUtil::addDays(DateUtil::today(), 1);
    auto audits = securityManager.queryAuditLogs(start, end, AuditSeverity::Info);
    std::cout << "近两日审计日志数量：" << audits.size() << '\n';
    for (const auto& audit : audits) {
        std::cout << audit.entryId << " | 操作者：" << audit.actorId
            << " | 动作：" << audit.action
            << " | 等级：" << static_cast<int>(audit.severity)
            << " | 详情：" << audit.details << '\n';
    }

    // 风险评估产生的告警
    securityManager.performRiskAssessment();
    auto alerts = securityManager.listAlerts(true);
    std::cout << "当前告警数量：" << alerts.size() << '\n';
    for (const auto& alert : alerts) {
        std::cout << "告警ID：" << alert.alertId << " | 读者：" << alert.readerId
            << " | 说明：" << alert.description
            << " | 已确认：" << (alert.acknowledged ? "是" : "否") << '\n';
        if (!alert.acknowledged) {
            securityManager.acknowledgeAlert(alert.alertId);
        }
    }

    // 注销会话 / 撤销角色
    securityManager.closeSession(sessionId);
    securityManager.revokeRole("R001", "ADMIN");
    std::cout << "安全管理演示完毕。\n";
}

void demoMultiTerminalSync(MultiTerminalSync& multiSync) {
    std::cout << "\n==================== 多终端同步演示 ====================\n";

    Date syncDate = DateUtil::addDays(DateUtil::today(), 1);
    multiSync.scheduleSync("DEV-001", syncDate);
    auto scheduled = multiSync.getScheduledSync("DEV-001");
    std::cout << "DEV-001 的计划同步日期："
        << (scheduled.has_value() ? DateUtil::toString(scheduled.value()) : "尚未计划") << '\n';
    multiSync.cancelScheduledSync("DEV-001");
    std::cout << "已取消 DEV-001 的计划同步。\n";

    auto conflicts = multiSync.detectConflicts("DEV-001");
    std::cout << "检测到的冲突条目数量：" << conflicts.size() << "（当前示例均为空）\n";
    multiSync.resolveConflicts("DEV-001", conflicts);

    multiSync.setAutoSyncInterval(std::chrono::minutes(45));
    multiSync.enableAutoSync(true);
    std::cout << "自动同步状态：" << (multiSync.isAutoSyncEnabled() ? "已启用" : "未启用") << '\n';
}

void demoReportGenerator(ReportGenerator& reportGenerator, BorrowService& borrowService) {
    std::cout << "\n==================== 报表生成演示 ====================\n";

    auto todayReport = reportGenerator.generateDailyReport(DateUtil::today());
    std::cout << "今日运营日报（节选）：\n" << todayReport.substr(0, 120) << "...（全文已生成）\n";
    auto monthlyReport = reportGenerator.generateMonthlyReport(DateUtil::today().year, DateUtil::today().month);
    reportGenerator.exportReportToFile(monthlyReport, "./demo_data/monthly_report_full.txt");
    reportGenerator.exportReportToHTML(monthlyReport, "./demo_data/monthly_report_full.html");
    reportGenerator.exportReportToMarkdown(monthlyReport, "./demo_data/monthly_report_full.md");
    std::cout << "已导出月报至文本/HTML/Markdown 文件。\n";

    std::vector<ReportSection> customSections = {
        {"服务概览", "场馆开放时间延长至 22:00，新增自习座位 50 个。"},
        {"重点项目", "开展“编程之夜”系列阅读活动，参与读者超 120 人次。"},
        {"改进计划", "计划上线智能问答机器人，提升信息咨询效率。"}
    };
    auto customReport = reportGenerator.generateCustomReport("季度服务总结", customSections);
    std::cout << "自定义报告已生成，标题：" << customReport << '\n';

    auto topCategories = reportGenerator.topBorrowedCategories(3);
    std::cout << "借阅量最高的图书类别：\n";
    for (const auto& item : topCategories) {
        std::cout << "- " << item.first << " -> 占比 "
            << std::fixed << std::setprecision(2) << item.second * 100 << "%\n";
    }

    auto topReaders = reportGenerator.topActiveReaders(3);
    std::cout << "活跃读者 TOP3：\n";
    for (const auto& item : topReaders) {
        std::cout << "- " << item.first << " -> 借阅 " << item.second << " 次\n";
    }

    reportGenerator.scheduleReportDelivery("季度服务总结", DateUtil::addDays(DateUtil::today(), 2));
    std::cout << "已将《季度服务总结》纳入报表发送计划。\n";
}

void demoFileManager(FileManager& fileManager, LibraryManager& libraryManager, BookService& bookService) {
    std::cout << "\n==================== 数据持久化（文件管理）演示 ====================\n";

    const std::filesystem::path dataDir = fileManager.getDataDirectory();

    fileManager.saveBooksToFile("books_snapshot.dat");
    fileManager.saveReadersToFile("readers_snapshot.dat");
    fileManager.saveBorrowRecordsToFile("records_snapshot.dat");
    fileManager.saveAllData();
    std::cout << "已保存图书 / 读者 / 借阅数据至 data 目录。\n";

    fileManager.loadAllData();
    std::cout << "已重新加载全部数据（若有改动会自动覆盖更新）。\n";

    fileManager.createAutoBackup();
    auto backups = fileManager.listAvailableBackups();
    std::cout << "自动备份数量：" << backups.size() << '\n';
    if (!backups.empty()) {
        fileManager.restoreFromBackup(backups.back());
        std::cout << "已恢复最新备份：" << backups.back() << '\n';
    }

    fileManager.exportToCSV("library_export.csv");
    fileManager.exportToJSON("library_export.json");
    std::cout << "已导出 CSV 与 JSON 文件。\n";

    // 构造一个简单的 CSV 用于演示导入
    std::ofstream csvFile(dataDir / "demo_import.csv");
    csvFile << "entity,payload\n";
    Book importBook("978-8888888888", "导入测试图书", "韩梅", "高校出版社", 2025, "测试导入", 1);
    csvFile << "book," << importBook.serialize() << '\n';
    csvFile.close();
    fileManager.importFromCSV("demo_import.csv");
    std::cout << "已从 demo_import.csv 导入一条测试图书记录。\n";

    bool valid = fileManager.validateDataIntegrity();
    std::cout << "数据完整性检查结果：" << (valid ? "通过" : "未通过") << '\n';
    if (!valid) {
        fileManager.repairCorruptedData();
        std::cout << "尝试修复数据一致性。\n";
    }

    std::cout << "文件管理演示完毕。为保持索引一致，手动移除导入的测试图书。\n";
    bookService.deleteBook("978-8888888888");
}

/* -------------------- 加密与日志演示 -------------------- */
void demoEncryptionAndLogger() {
    std::cout << "\n==================== 加密工具与日志演示 ====================\n";
    std::string salt = Encryption::generateSalt();
    std::string hash = Encryption::hashPassword("SecretPassword123", salt);
    bool ok = Encryption::verifyPassword("SecretPassword123", salt, hash);
    std::cout << "口令校验结果：" << (ok ? "成功" : "失败") << '\n';

    Logger::instance().debug("调试日志：便于排查细节问题。");
    Logger::instance().info("信息日志：系统运行概览。");
    Logger::instance().warn("警告日志：需要留意的操作。");
    Logger::instance().error("错误日志：请及时处理。");
}

int main() {
    Logger::instance().setLogLevel(LogLevel::Debug);
    Logger::instance().info("图书馆管理系统启动……");
    LibraryManager libraryManager;
    FileManager fileManager(libraryManager, "./demo_data");
    BookService bookService(libraryManager);
    ReaderService readerService(libraryManager);
    BorrowService borrowService(libraryManager);
    SearchService searchService(bookService, readerService);
    AdvancedSearch advancedSearch(bookService, readerService);
    AIRecommender aiRecommender(bookService, borrowService, readerService, searchService);
    DataAnalyzer dataAnalyzer(bookService, borrowService, readerService);
    NotificationCenter notificationCenter(readerService, borrowService, bookService);
    ReservationSystem reservationSystem(bookService, readerService, borrowService);
    CreditSystem creditSystem(readerService, borrowService);
    CommunityManager communityManager(bookService, readerService, notificationCenter);
    MultiTerminalSync multiSync(libraryManager, readerService, borrowService, fileManager);
    ReportGenerator reportGenerator(bookService, readerService, borrowService,
        dataAnalyzer, fileManager, notificationCenter);
    SecurityManager securityManager(readerService, borrowService, fileManager);
    const bool loadedFromStorage = fileManager.loadAllData();
    if (loadedFromStorage) {
        Logger::instance().info("已自动加载持久化数据。");
    }
    else {
        Logger::instance().warn("自动加载持久化数据失败或未检测到历史数据，将使用演示初始数据。");
    }
    // 初始化演示数据
    if (bookService.searchByKeyword("").empty()) {
        std::vector<Book> baseBooks = {
            Book("978-7302598980", "C++ Primer Plus", "Stephen Prata", "人民邮电出版社", 2021, "Programming", 5),
            Book("978-7121360985", "Effective Modern C++", "Scott Meyers", "机械工业出版社", 2019, "Programming", 3),
            Book("978-0131103627", "The C Programming Language", "Kernighan & Ritchie", "Prentice Hall", 1988, "Programming", 4),
            Book("978-0262033848", "Introduction to Algorithms", "CLRS", "MIT Press", 2009, "Algorithms", 6),
            Book("978-0596007126", "Head First Design Patterns", "Eric Freeman", "O'Reilly Media", 2004, "Design", 2)
        };
        for (const auto& book : baseBooks) bookService.addBook(book);
    }
    if (readerService.getActiveReaderCount() == 0) {
        std::vector<Reader> baseReaders = {
            Reader("R001", "Alice", {"13800000001", "alice@example.com", "北京市海淀区"}, 5, 25),
            Reader("R002", "Bob", {"13800000002", "bob@example.com", "上海市浦东新区"}, 6, 30),
            Reader("R003", "Charlie", {"13800000003", "charlie@example.com", "广州市天河区"}, 4, 22)
        };
        for (const auto& reader : baseReaders) readerService.registerReader(reader);
    }
    auto historyR001 = borrowService.getReaderBorrowHistory("R001");
    auto historyR002 = borrowService.getReaderBorrowHistory("R002");
    auto historyR003 = borrowService.getReaderBorrowHistory("R003");
    if (historyR001.empty() && historyR002.empty() && historyR003.empty()) {
        borrowService.borrowBook("R001", "978-7302598980");
        borrowService.borrowBook("R002", "978-0262033848");
        borrowService.borrowBook("R002", "978-7121360985");
        borrowService.borrowBook("R003", "978-0596007126");
    }
    std::cout << "\n==================== 运行模式选择 ====================\n"
        << " 1. 演示模式（执行原有全功能演示，输出大量示例信息）\n"
        << " 2. 交互模式（进入命令行菜单，按需操作各项功能）\n"
        << "请输入选项（默认 1）：";
    std::string modeInput;
    std::getline(std::cin, modeInput);
    int mode = 1;
    if (!modeInput.empty()) {
        try {
            mode = std::stoi(modeInput);
        }
        catch (...) {
            mode = 1;
        }
    }
    if (mode == 2) {
        std::cout << "您选择了交互模式。可随时通过菜单执行各项功能。\n";
        InteractiveShell shell(bookService,
            readerService,
            borrowService,
            searchService,
            advancedSearch,
            aiRecommender,
            reservationSystem,
            notificationCenter,
            creditSystem,
            communityManager,
            dataAnalyzer,
            reportGenerator,
            fileManager,
            securityManager,
            multiSync);
        shell.run();
    }
    else {
        std::cout << "您选择了演示模式，即将按照预设流程输出示例。\n";
        printBookList(bookService.searchByKeyword(""), "初始全部图书");
        printReaderList(readerService.getReadersByBorrowCount(0), "初始全部读者");
        demoBookServiceExtended(bookService, libraryManager);
        demoReaderServiceExtended(readerService);
        demoBorrowServiceExtended(borrowService, bookService, readerService, libraryManager);
        demoSearchAndAdvanced(searchService, advancedSearch, bookService, readerService, borrowService);
        demoAIRecommender(aiRecommender, bookService);
        demoDataAnalyzer(dataAnalyzer);
        ReservationDemoContext reservationContext = demoReservationSystem(reservationSystem, bookService,
            readerService, borrowService);
        demoNotificationCenter(notificationCenter, borrowService, readerService, libraryManager, reservationContext);
        demoCreditSystem(creditSystem, readerService);
        CommunityDemoContext communityContext = demoCommunityManager(communityManager, notificationCenter);
        demoSecurityManager(securityManager);
        demoMultiTerminalSync(multiSync);
        demoReportGenerator(reportGenerator, borrowService);
        demoFileManager(fileManager, libraryManager, bookService);
        demoEncryptionAndLogger();
    }
    Logger::instance().info("图书馆管理系统程序结束。");
    return 0;
}
