
#include "InteractiveShell.h"
#include "BorrowRecord.h"
#include "DateUtil.h"
#include "Reader.h"
#include "StringUtil.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

    // 管理员默认密码
    constexpr const char* kAdminPassword = "1024";

    // 辅助函数：打印图书列表
    void printBookList(const std::vector<Book>& books, const std::string& title) {
        std::cout << "\n==== " << title << "（共 " << books.size() << " 本）====\n";
        for (const auto& book : books) {
            std::cout << "ISBN：" << book.getISBN()
                << " | 书名：" << book.getTitle()
                << " | 作者：" << book.getAuthor()
                << " | 出版年份：" << book.getPublishYear()
                << " | 库存：" << book.getStock()
                << " | 类别：" << book.getCategory() << '\n';
        }
        if (books.empty()) std::cout << "（暂无数据）\n";
    }

    // 辅助函数：打印读者列表
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
        if (readers.empty()) std::cout << "（暂无数据）\n";
    }

    // 辅助函数：打印借阅记录
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
                << " | 当前罚金：" << std::fixed << std::setprecision(2) << record.getFine() << '\n';
        }
        if (records.empty()) std::cout << "（暂无数据）\n";
    }

    // 将借阅状态枚举转换为字符串
    std::string borrowStatusToString(BorrowStatus status) {
        switch (status) {
        case BorrowStatus::Success: return "成功";
        case BorrowStatus::ReaderNotFound: return "读者不存在";
        case BorrowStatus::BookNotFound: return "图书不存在";
        case BorrowStatus::BookOutOfStock: return "库存不足";
        case BorrowStatus::LimitExceeded: return "超出借书上限";
        case BorrowStatus::AlreadyBorrowed: return "已借阅";
        case BorrowStatus::SuspendedAccount: return "账户暂停";
        case BorrowStatus::ReservationRequired: return "需要预约";
        default: return "失败";
        }
    }

    // 将归还状态枚举转换为字符串
    std::string returnStatusToString(ReturnStatus status) {
        switch (status) {
        case ReturnStatus::Success: return "成功";
        case ReturnStatus::ReaderNotFound: return "读者不存在";
        case ReturnStatus::BookNotFound: return "图书不存在";
        case ReturnStatus::RecordNotFound: return "记录不存在";
        case ReturnStatus::AlreadyReturned: return "已归还";
        default: return "失败";
        }
    }

    // 将审计级别枚举转换为字符串
    std::string severityToString(AuditSeverity severity) {
        switch (severity) {
        case AuditSeverity::Info: return "Info";
        case AuditSeverity::Warning: return "Warning";
        case AuditSeverity::Critical: return "Critical";
        default: return "Unknown";
        }
    }

} // namespace

// 构造函数：初始化所有服务依赖
InteractiveShell::InteractiveShell(BookService& bookService,
    ReaderService& readerService,
    BorrowService& borrowService,
    SearchService& searchService,
    AdvancedSearch& advancedSearch,
    AIRecommender& recommender,
    ReservationSystem& reservationSystem,
    NotificationCenter& notificationCenter,
    CreditSystem& creditSystem,
    CommunityManager& communityManager,
    DataAnalyzer& dataAnalyzer,
    ReportGenerator& reportGenerator,
    FileManager& fileManager,
    SecurityManager& securityManager,
    MultiTerminalSync& multiSync)
    : m_bookService(bookService)
    , m_readerService(readerService)
    , m_borrowService(borrowService)
    , m_searchService(searchService)
    , m_advancedSearch(advancedSearch)
    , m_recommender(recommender)
    , m_reservationSystem(reservationSystem)
    , m_notificationCenter(notificationCenter)
    , m_creditSystem(creditSystem)
    , m_communityManager(communityManager)
    , m_dataAnalyzer(dataAnalyzer)
    , m_reportGenerator(reportGenerator)
    , m_fileManager(fileManager)
    , m_securityManager(securityManager)
    , m_multiSync(multiSync)
{
}

// 主运行循环
void InteractiveShell::run() {
    showWelcome();
    while (m_session.running) {
        // 如果未登录，显示登录菜单
        if (m_session.role == CliSession::Role::None) {
            handleLoginMenu();
        }
        else {
            // 已登录，显示主菜单
            mainMenu();
        }
    }
    std::cout << "感谢使用图书馆管理系统，再见！\n";
}

/* --- 顶层流程 --- */

void InteractiveShell::showWelcome() const {
    std::cout << "========================================\n";
    std::cout << " 欢迎使用图书馆管理系统（交互模式）\n";
    std::cout << "========================================\n";
}

// 处理登录逻辑
void InteractiveShell::handleLoginMenu() {
    while (m_session.role == CliSession::Role::None && m_session.running) {
        printSeparator();
        std::cout << "请选择身份：\n"
            << " 1. 普通读者\n"
            << " 2. 管理员\n"
            << " 0. 退出系统\n";
        int choice = readInt("输入选项：", 0, 2);
        switch (choice) {
        case 1: {
            // 读者登录
            std::string readerId = readLine("请输入读者ID：");
            if (m_readerService.checkReaderIdExists(readerId)) {
                m_session.role = CliSession::Role::Reader;
                m_session.readerId = readerId;
                std::cout << "已登录读者：" << readerId << "\n";
            }
            else {
                std::cout << "读者ID不存在，请重试。\n";
            }
            break;
        }
        case 2:
            // 管理员登录
            if (authenticateAdmin()) {
                m_session.role = CliSession::Role::Admin;
                m_session.readerId.reset();
                std::cout << "管理员登录成功。\n";
            }
            else {
                std::cout << "密码错误。\n";
            }
            break;
        case 0:
            m_session.running = false;
            break;
        default:
            break;
        }
    }
}

// 验证管理员密码
bool InteractiveShell::authenticateAdmin() {
    const std::string password = readLine("请输入管理员密码：");
    return password == kAdminPassword;
}

// 确保当前会话是读者身份
bool InteractiveShell::ensureReaderLoggedIn() {
    if (m_session.role != CliSession::Role::Reader || !m_session.readerId.has_value()) {
        std::cout << "当前操作仅限读者账户使用，请先登录读者身份。\n";
        return false;
    }
    return true;
}

// 根据角色分发主菜单
void InteractiveShell::mainMenu() {
    if (m_session.role == CliSession::Role::Reader) {
        readerMenu();
    }
    else if (m_session.role == CliSession::Role::Admin) {
        adminMenu();
    }
}

/* --- 普通读者菜单 --- */

void InteractiveShell::readerMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Reader) {
        printSeparator();
        std::cout << "读者功能菜单：\n"
            << " 1. 图书查询\n"
            << " 2. 借阅服务\n"
            << " 3. 预约服务\n"
            << " 4. 推荐系统\n"
            << " 5. 通知中心\n"
            << " 9. 切换身份\n"
            << " 0. 退出系统\n";
        int choice = readInt("输入选项：", 0, 9);
        switch (choice) {
        case 1: searchMenu(); break;
        case 2: borrowMenu(); break;
        case 3: reservationMenu(); break;
        case 4: recommenderMenu(); break;
        case 5: notificationMenu(); break;
        case 9:
            m_session.reset(); // 注销
            stay = false;
            break;
        case 0:
            m_session.running = false;
            stay = false;
            break;
        default:
            break;
        }
    }
}

// 读者：查询菜单
void InteractiveShell::searchMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Reader) {
        printSeparator();
        std::cout << "图书查询：\n"
            << " 1. 按作者查询\n"
            << " 2. 按标题查询\n"
            << " 3. 关键词查询\n"
            << " 4. 高级（语义）搜索\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 4);
        switch (choice) {
        case 1: performSearchByAuthor(); break;
        case 2: performSearchByTitle(); break;
        case 3: performSearchByKeyword(); break;
        case 4: performAdvancedSearch(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 读者：借阅菜单
void InteractiveShell::borrowMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Reader) {
        printSeparator();
        std::cout << "借阅服务：\n"
            << " 1. 借书\n"
            << " 2. 还书\n"
            << " 3. 续借\n"
            << " 4. 查看当前在借\n"
            << " 5. 查看历史记录\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 5);
        switch (choice) {
        case 1: performBorrow(); break;
        case 2: performReturn(); break;
        case 3: performRenew(); break;
        case 4:
            if (ensureReaderLoggedIn()) {
                auto records = m_borrowService.getCurrentBorrows(m_session.readerId.value());
                printBorrowRecords(records, "当前在借列表");
                pauseAndWait();
            }
            break;
        case 5:
            if (ensureReaderLoggedIn()) {
                auto records = m_borrowService.getReaderBorrowHistory(m_session.readerId.value());
                printBorrowRecords(records, "借阅历史");
                pauseAndWait();
            }
            break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 读者：预约菜单
void InteractiveShell::reservationMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Reader) {
        printSeparator();
        std::cout << "预约服务：\n"
            << " 1. 预约图书\n"
            << " 2. 查看某图书预约队列\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 2);
        switch (choice) {
        case 1: performReservation(); break;
        case 2:
            if (ensureReaderLoggedIn()) {
                const std::string isbn = readLine("请输入 ISBN：");
                auto queue = m_reservationSystem.getReservationQueue(isbn);
                std::cout << "当前预约队列（ISBN: " << isbn << "）\n";
                if (queue.empty()) std::cout << "（暂无预约）\n";
                else {
                    int pos = 0;
                    for (const auto& readerId : queue) std::cout << ++pos << ". " << readerId << '\n';
                }
                pauseAndWait();
            }
            break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 读者：推荐菜单
void InteractiveShell::recommenderMenu() {
    if (!ensureReaderLoggedIn()) return;

    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Reader) {
        printSeparator();
        std::cout << "推荐系统：\n"
            << " 1. 个性化推荐\n"
            << " 2. 基于历史记录推荐\n"
            << " 3. 查看热门图书\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 3);
        switch (choice) {
        case 1: performViewRecommendations(); break;
        case 2: {
            auto books = m_recommender.recommendBasedOnHistory(m_session.readerId.value());
            printBookList(books, "历史记录相关推荐");
            pauseAndWait();
            break;
        }
        case 3: {
            auto books = m_recommender.getTopBorrowedBooks(30, 5);
            printBookList(books, "近 30 天热门图书");
            pauseAndWait();
            break;
        }
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 读者：通知菜单
void InteractiveShell::notificationMenu() {
    if (!ensureReaderLoggedIn()) return;

    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Reader) {
        printSeparator();
        std::cout << "通知中心：\n"
            << " 1. 查看未读通知\n"
            << " 2. 查看通知统计\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 2);
        switch (choice) {
        case 1: performViewNotifications(); break;
        case 2: {
            auto stats = m_notificationCenter.getNotificationStatistics();
            std::cout << "通知统计：发送 " << stats.sentCount
                << " 条，送达 " << stats.deliveredCount
                << " 条，已读 " << stats.readCount << " 条。\n";
            pauseAndWait();
            break;
        }
        case 0: stay = false; break;
        default: break;
        }
    }
}

/* --- 管理员菜单 --- */

void InteractiveShell::adminMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "管理员功能菜单：\n"
            << " 1. 图书管理\n"
            << " 2. 读者管理\n"
            << " 3. 借阅管理\n"
            << " 4. 预约管理\n"
            << " 5. 通知中心\n"
            << " 6. 信用体系\n"
            << " 7. 数据洞察 / 推荐\n"
            << " 8. 社区管理\n"
            << " 9. 多终端同步\n"
            << "10. 数据持久化\n"
            << "11. 安全与审计\n"
            << "99. 切换身份\n"
            << " 0. 退出系统\n";
        int choice = readInt("输入选项：", 0, 99);
        switch (choice) {
        case 1: adminBookMenu(); break;
        case 2: adminReaderMenu(); break;
        case 3: adminBorrowMenu(); break;
        case 4: adminReservationMenu(); break;
        case 5: adminNotificationMenu(); break;
        case 6: adminCreditMenu(); break;
        case 7: adminInsightsMenu(); break;
        case 8: adminCommunityMenu(); break;
        case 9: adminMultiSyncMenu(); break;
        case 10: adminDataMenu(); break;
        case 11: adminSecurityMenu(); break;
        case 99:
            m_session.reset(); // 注销
            stay = false;
            break;
        case 0:
            m_session.running = false;
            stay = false;
            break;
        default:
            std::cout << "非法选项。\n";
            break;
        }
    }
}

// 管理员：图书管理子菜单
void InteractiveShell::adminBookMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "图书管理：\n"
            << " 1. 新增图书\n"
            << " 2. 删除图书\n"
            << " 3. 调整库存\n"
            << " 4. 修改图书信息\n"
            << " 5. 查看分类统计\n"
            << " 6. 查询（作者/标题/类别/多条件）\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 6);
        switch (choice) {
        case 1: performAdminAddBook(); break;
        case 2: performAdminDeleteBook(); break;
        case 3: performAdminAdjustStock(); break;
        case 4: performAdminEditBook(); break;

        case 5: {                                       // 查看分类统计
            auto dist = m_bookService.getCategoryDistribution();
            std::cout << "\n==== 分类统计 ====\n";
            for (const auto& [category, count] : dist) {
                std::cout << category << " -> " << count << '\n';
            }
            pauseAndWait();
            break;
        }

        case 6: {                                       // 综合查询
            std::string keyword = readLine("请输入关键词（留空输出全部）：");
            if (keyword.empty()) {
                printBookList(m_bookService.searchByKeyword(""), "全部图书列表");
            }
            else {
                auto byAuthor = m_bookService.searchByAuthor(keyword);
                auto byTitle = m_bookService.searchByTitle(keyword);
                auto byCategory = m_bookService.searchByCategory(keyword);
                printBookList(byTitle, "标题包含关键词的图书");
                printBookList(byAuthor, "作者包含关键词的图书");
                printBookList(byCategory, "类别包含关键词的图书");

                // 演示多条件搜索
                SearchCriteria criteria;
                criteria.title = keyword;
                criteria.inStockOnly = true;
                auto combined = m_bookService.searchByMultipleCriteria(criteria);
                printBookList(combined, "多条件（标题 + 在库）查询结果");
            }
            pauseAndWait();
            break;
        }

        case 0:
            stay = false;
            break;
        default:
            break;
        }
    }
}


// 管理员：读者管理子菜单
void InteractiveShell::adminReaderMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "读者管理：\n"
            << " 1. 注册读者\n"
            << " 2. 修改读者信息\n"
            << " 3. 注销读者\n"
            << " 4. 查看活跃读者数\n"
            << " 5. 借阅排行榜\n"
            << " 6. 年龄分布\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 6);
        switch (choice) {
        case 1: performAdminRegisterReader(); break;
        case 2: performAdminUpdateReader(); break;
        case 3: performAdminDeleteReader(); break;
        case 4: {
            int count = m_readerService.getActiveReaderCount();
            std::cout << "当前活跃读者数量：" << count << '\n';
            pauseAndWait();
            break;
        }
        case 5: {
            auto top = m_readerService.getTopBorrowers(10);
            printReaderList(top, "借阅排行榜 TOP10");
            pauseAndWait();
            break;
        }
        case 6: {
            auto dist = m_readerService.getReaderAgeDistribution();
            std::cout << "\n==== 年龄分布 ====\n";
            for (const auto& [age, num] : dist) {
                std::cout << age << " 岁 -> " << num << " 人\n";
            }
            pauseAndWait();
            break;
        }
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：借阅管理子菜单
void InteractiveShell::adminBorrowMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "借阅管理：\n"
            << " 1. 查看某读者历史借阅\n"
            << " 2. 查看某图书借阅历史\n"
            << " 3. 查看逾期列表\n"
            << " 4. 查看统计（按日/按月）\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 4);
        switch (choice) {
        case 1: performAdminBorrowHistoryByReader(); break;
        case 2: performAdminBorrowHistoryByBook(); break;
        case 3: performAdminBorrowOverdue(); break;
        case 4: performAdminBorrowStats(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：预约管理子菜单
void InteractiveShell::adminReservationMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "预约管理：\n"
            << " 1. 查看图书预约队列\n"
            << " 2. 处理可发放预约\n"
            << " 3. 取消读者预约\n"
            << " 4. 设置预约优先级/调整队列\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 4);
        switch (choice) {
        case 1: performAdminReservationQueue(); break;
        case 2: performAdminReservationProcess(); break;
        case 3: performAdminReservationCancel(); break;
        case 4: performAdminReservationPriority(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：通知管理子菜单
void InteractiveShell::adminNotificationMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "通知中心：\n"
            << " 1. 向读者发送到期提醒\n"
            << " 2. 发送自定义通知\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 2);
        switch (choice) {
        case 1: performAdminSendDueReminder(); break;
        case 2: performAdminSendCustomNotification(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：信用管理子菜单
void InteractiveShell::adminCreditMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "信用体系：\n"
            << " 1. 查看读者信用分\n"
            << " 2. 调整读者积分（加/扣）\n"
            << " 3. 暂停读者借阅\n"
            << " 4. 恢复读者借阅\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 4);
        switch (choice) {
        case 1: performAdminCreditView(); break;
        case 2: performAdminCreditAdjust(); break;
        case 3: performAdminCreditSuspend(); break;
        case 4: performAdminCreditRestore(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：数据洞察子菜单
void InteractiveShell::adminInsightsMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "数据洞察 / 推荐：\n"
            << " 1. 生成今日日报\n"
            << " 2. 生成本月报告\n"
            << " 3. 借阅趋势分析（近30天）\n"
            << " 4. 未来热门图书预测\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 4);
        switch (choice) {
        case 1: performAdminGenerateReport(); break;
        case 2: performAdminMonthlyReport(); break;
        case 3: performAdminBorrowTrend(); break;
        case 4: performAdminPredictPopular(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：社区管理子菜单
void InteractiveShell::adminCommunityMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "社区管理：\n"
            << " 1. 查看社区统计\n"
            << " 2. 创建讨论串\n"
            << " 3. 查看某图书讨论串\n"
            << " 4. 关闭讨论串\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 4);
        switch (choice) {
        case 1: performAdminCommunityStats(); break;
        case 2: performAdminCreateThread(); break;
        case 3: performAdminListThreads(); break;
        case 4: performAdminCloseThread(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：同步管理子菜单
void InteractiveShell::adminMultiSyncMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "多终端同步：\n"
            << " 1. 注册设备\n"
            << " 2. 查看设备列表\n"
            << " 3. 安排同步\n"
            << " 4. 取消同步\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 4);
        switch (choice) {
        case 1: performAdminRegisterDevice(); break;
        case 2: performAdminListDevices(); break;
        case 3: performAdminScheduleSync(); break;
        case 4: performAdminCancelSync(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：数据持久化子菜单
void InteractiveShell::adminDataMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "数据持久化：\n"
            << " 1. 保存全部数据\n"
            << " 2. 载入全部数据\n"
            << " 3. 导出 CSV\n"
            << " 4. 导出 JSON\n"
            << " 5. 创建自动备份\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 5);
        switch (choice) {
        case 1: performAdminSaveData(); break;
        case 2: performAdminLoadData(); break;
        case 3: performAdminExportCSV(); break;
        case 4: performAdminExportJSON(); break;
        case 5: performAdminBackup(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

// 管理员：安全审计子菜单
void InteractiveShell::adminSecurityMenu() {
    bool stay = true;
    while (stay && m_session.running && m_session.role == CliSession::Role::Admin) {
        printSeparator();
        std::cout << "安全与审计：\n"
            << " 1. 查看审计日志\n"
            << " 2. 风险评估与告警\n"
            << " 0. 返回上一层\n";
        int choice = readInt("输入选项：", 0, 2);
        switch (choice) {
        case 1: performAdminAuditLogs(); break;
        case 2: performAdminRiskAssessment(); break;
        case 0: stay = false; break;
        default: break;
        }
    }
}

/* --- 读者增改删 --- */

void InteractiveShell::performAdminRegisterReader() {
    std::string readerId = readLine("读者ID：");
    if (readerId.empty()) {
        std::cout << "ID 不能为空。\n";
        pauseAndWait();
        return;
    }
    if (m_readerService.checkReaderIdExists(readerId)) {
        std::cout << "该读者ID已存在。\n";
        pauseAndWait();
        return;
    }

    std::string name = readLine("姓名：");
    std::string phone = readLine("电话：");
    std::string email = readLine("邮箱：");
    std::string address = readLine("地址：");
    int maxBorrow = readInt("最大借书数：", 1, 50);
    int age = readInt("年龄（0-120）：", 0, 120);

    Reader reader(readerId, name, { phone, email, address }, maxBorrow, age);
    if (m_readerService.registerReader(reader)) {
        std::cout << "读者注册成功。\n";
    }
    else {
        std::cout << "注册失败。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminUpdateReader() {
    std::string readerId = readLine("请输入要修改的读者ID：");
    Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        std::cout << "读者不存在。\n";
        pauseAndWait();
        return;
    }

    ReaderUpdate update;
    std::string name = readLine("新姓名（回车跳过）：");
    if (!name.empty()) {
        update.name = name;
    }

    std::string updateContact = readLine("是否更新联系方式？(y/n)：");
    if (!updateContact.empty() && (updateContact[0] == 'y' || updateContact[0] == 'Y')) {
        std::string phone = readLine("电话：");
        std::string email = readLine("邮箱：");
        std::string address = readLine("地址：");
        update.contact = ContactInfo{ phone, email, address };
    }

    std::string maxBorrowStr = readLine("新最大借书数（回车跳过）：");
    if (!maxBorrowStr.empty()) {
        try {
            int maxBorrow = std::stoi(maxBorrowStr);
            if (maxBorrow > 0) {
                update.maxBorrow = maxBorrow;
            }
        }
        catch (...) {
            std::cout << "忽略非法的最大借书数输入。\n";
        }
    }

    std::string ageStr = readLine("新年龄（回车跳过）：");
    if (!ageStr.empty()) {
        try {
            int ageNum = std::stoi(ageStr);
            if (ageNum >= 0 && ageNum <= 120) {
                update.age = ageNum;
            }
        }
        catch (...) {
            std::cout << "忽略非法年龄输入。\n";
        }
    }

    if (!update.name && !update.contact && !update.maxBorrow && !update.age) {
        std::cout << "未修改任何字段。\n";
    }
    else if (m_readerService.updateReaderInfo(readerId, update)) {
        std::cout << "读者信息修改成功。\n";
    }
    else {
        std::cout << "修改失败。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminDeleteReader() {
    std::string readerId = readLine("请输入要注销的读者ID：");
    if (!m_readerService.checkReaderIdExists(readerId)) {
        std::cout << "读者不存在。\n";
    }
    else if (m_readerService.permanentlyDeleteReader(readerId)) {
        std::cout << "读者已注销。\n";
    }
    else {
        std::cout << "注销失败。\n";
    }
    pauseAndWait();
}

/* --- 普通读者操作 --- */

void InteractiveShell::performSearchByAuthor() {
    const std::string author = readLine("请输入作者关键词：");
    auto books = m_bookService.searchByAuthor(author);
    printBookList(books, "按作者查询结果");
    pauseAndWait();
}

void InteractiveShell::performSearchByTitle() {
    const std::string title = readLine("请输入标题关键词：");
    auto books = m_bookService.searchByTitle(title);
    printBookList(books, "按标题查询结果");
    pauseAndWait();
}

void InteractiveShell::performSearchByKeyword() {
    const std::string keyword = readLine("请输入关键词：");
    auto books = m_bookService.searchByKeyword(keyword);
    printBookList(books, "关键词查询结果");
    pauseAndWait();
}

void InteractiveShell::performAdvancedSearch() {
    std::string keyword = readLine("请输入高级搜索关键词：");
    auto books = m_advancedSearch.semanticSearch(keyword);
    printBookList(books, "高级搜索（语义）结果");
    pauseAndWait();
}

void InteractiveShell::performBorrow() {
    if (!ensureReaderLoggedIn()) return;

    std::string isbn = readLine("请输入要借阅的 ISBN：");
    BorrowResult result = m_borrowService.borrowBook(m_session.readerId.value(), isbn);
    std::cout << "借阅状态：" << borrowStatusToString(result.status)
        << " | " << result.message << '\n';
    if (result.record) {
        std::cout << "应还日期：" << DateUtil::toString(result.record->getDueDate()) << '\n';
    }
    pauseAndWait();
}

void InteractiveShell::performReturn() {
    if (!ensureReaderLoggedIn()) return;

    std::string isbn = readLine("请输入要归还的 ISBN：");
    ReturnResult result = m_borrowService.returnBook(m_session.readerId.value(), isbn);
    std::cout << "还书状态：" << returnStatusToString(result.status)
        << " | " << result.message;
    if (result.status == ReturnStatus::Success) {
        std::cout << " | 罚金：" << std::fixed << std::setprecision(2) << result.fine;
    }
    std::cout << '\n';
    pauseAndWait();
}

void InteractiveShell::performRenew() {
    if (!ensureReaderLoggedIn()) return;

    std::string isbn = readLine("请输入要续借的 ISBN：");
    bool success = m_borrowService.renewBook(m_session.readerId.value(), isbn);
    if (success) {
        std::cout << "续借成功。\n";
    }
    else {
        std::cout << "续借失败，请检查是否已达续借上限或图书未借阅。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performReservation() {
    if (!ensureReaderLoggedIn()) return;

    std::string isbn = readLine("请输入要预约的 ISBN：");
    ReservationResult result = m_reservationSystem.reserveBook(m_session.readerId.value(), isbn);
    if (result.success) {
        std::cout << "预约成功，预约编号：" << result.reservationId
            << " | 队列位置：" << result.positionInQueue << '\n';
    }
    else {
        std::cout << "预约失败：" << result.message << '\n';
    }
    pauseAndWait();
}

void InteractiveShell::performViewNotifications() {
    if (!ensureReaderLoggedIn()) return;

    auto unread = m_notificationCenter.getUnreadNotifications(m_session.readerId.value());
    std::cout << "\n==== 未读通知（" << unread.size() << " 条）====\n";
    for (const auto& notification : unread) {
        std::cout << "- [" << DateUtil::toString(notification.sentDate)
            << "] " << notification.message
            << "（ID：" << notification.id << "）\n";
    }
    if (unread.empty()) {
        std::cout << "暂无未读通知。\n";
    }
    else {
        std::string mark = readLine("是否将全部未读通知标记为已读？(y/n)：");
        if (!mark.empty() && (mark[0] == 'y' || mark[0] == 'Y')) {
            for (const auto& item : unread) {
                m_notificationCenter.markAsRead(item.id);
            }
            std::cout << "已全部标记为已读。\n";
        }
    }
    pauseAndWait();
}

void InteractiveShell::performViewRecommendations() {
    if (!ensureReaderLoggedIn()) return;

    auto books = m_recommender.recommendForReader(m_session.readerId.value(), 5);
    printBookList(books, "个性化推荐");
    pauseAndWait();
}

/* --- 管理员功能实现 --- */

void InteractiveShell::performAdminAddBook() {
    std::string isbn = readLine("请输入 ISBN：");
    std::string title = readLine("请输入书名：");
    std::string author = readLine("请输入作者：");
    std::string publisher = readLine("请输入出版社：");
    int publishYear = readInt("请输入出版年份：", 0, 9999);
    std::string category = readLine("请输入类别：");
    int stock = readInt("请输入初始库存：", 0, 100000);

    Book book(isbn, title, author, publisher, publishYear, category, stock);
    if (m_bookService.addBook(book)) {
        std::cout << "新增图书成功。\n";
    }
    else {
        std::cout << "新增失败，可能是 ISBN 已存在。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminEditBook() {
    std::string isbn = readLine("请输入要修改的图书 ISBN：");
    auto books = m_bookService.searchByISBN(isbn);
    if (books.empty()) {
        std::cout << "图书不存在。\n";
        pauseAndWait();
        return;
    }

    const Book& original = books.front();
    std::cout << "当前信息：\n"
        << " 书名：" << original.getTitle() << '\n'
        << " 作者：" << original.getAuthor() << '\n'
        << " 出版社：" << original.getPublisher() << '\n'
        << " 出版年份：" << original.getPublishYear() << '\n'
        << " 类别：" << original.getCategory() << '\n'
        << " 库存：" << original.getStock() << '\n';

    BookUpdate update;
    std::string input;

    // 逐项询问更新
    input = readLine("新书名（回车跳过）：");
    if (!input.empty()) update.title = input;

    input = readLine("新作者（回车跳过）：");
    if (!input.empty()) update.author = input;

    input = readLine("新出版社（回车跳过）：");
    if (!input.empty()) update.publisher = input;

    input = readLine("新类别（回车跳过）：");
    if (!input.empty()) update.category = input;

    input = readLine("新出版年份（回车跳过）：");
    if (!input.empty()) {
        try {
            int year = std::stoi(input);
            update.publishYear = year;
        }
        catch (...) {
            std::cout << "出版年份输入无效，忽略。\n";
        }
    }

    input = readLine("新库存（回车跳过）：");
    if (!input.empty()) {
        try {
            int stock = std::stoi(input);
            update.stock = stock;
        }
        catch (...) {
            std::cout << "库存输入无效，忽略。\n";
        }
    }

    input = readLine("新标签（以逗号分隔，回车跳过）：");
    if (!input.empty()) {
        auto parts = StringUtil::split(input, ',');
        for (auto& part : parts) part = StringUtil::trim(part);
        update.tags = parts;
    }

    if (!update.title && !update.author && !update.publisher &&
        !update.category && !update.publishYear && !update.stock && !update.tags) {
        std::cout << "未修改任何字段。\n";
    }
    else if (m_bookService.updateBookInfo(isbn, update)) {
        std::cout << "图书信息已更新。\n";
    }
    else {
        std::cout << "更新失败。\n";
    }
    pauseAndWait();
}


void InteractiveShell::performAdminDeleteBook() {
    std::string isbn = readLine("请输入要删除的 ISBN：");
    if (m_bookService.deleteBook(isbn)) {
        std::cout << "图书已删除。\n";
    }
    else {
        std::cout << "删除失败，可能是 ISBN 不存在。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminAdjustStock() {
    std::string isbn = readLine("请输入 ISBN：");
    std::cout << "请选择库存操作：\n"
        << " 1. 增加库存\n"
        << " 2. 减少库存\n"
        << " 3. 设置库存\n";
    int choice = readInt("输入选项：", 1, 3);
    int amount = readInt("请输入数量：", 0, 100000);
    bool success = false;

    switch (choice) {
    case 1: success = m_bookService.increaseStock(isbn, amount); break;
    case 2: success = m_bookService.decreaseStock(isbn, amount); break;
    case 3: success = m_bookService.setStock(isbn, amount); break;
    default: break;
    }

    if (success) std::cout << "库存调整成功。\n";
    else std::cout << "库存调整失败，请检查 ISBN 是否存在。\n";
    pauseAndWait();
}

/* --- 借阅管理 --- */

void InteractiveShell::performAdminBorrowHistoryByReader() {
    std::string readerId = readLine("请输入读者ID：");
    auto records = m_borrowService.getReaderBorrowHistory(readerId);
    printBorrowRecords(records, "读者借阅历史");
    pauseAndWait();
}

void InteractiveShell::performAdminBorrowHistoryByBook() {
    std::string isbn = readLine("请输入 ISBN：");
    auto records = m_borrowService.getBookBorrowHistory(isbn);
    printBorrowRecords(records, "图书借阅历史");
    pauseAndWait();
}

void InteractiveShell::performAdminBorrowStats() {
    std::cout << "统计类型：\n"
        << " 1. 某日\n"
        << " 2. 某月\n";
    int type = readInt("输入选项：", 1, 2);
    if (type == 1) {
        std::string dateStr = readLine("请输入日期（YYYY-MM-DD）：");
        Date date = DateUtil::fromString(dateStr);
        auto stats = m_borrowService.getDailyStatistics(date);
        std::cout << "借出：" << stats.totalBorrowed
            << " | 归还：" << stats.totalReturned
            << " | 逾期：" << stats.overdueCount
            << " | 罚金：" << std::fixed << std::setprecision(2) << stats.totalFines << '\n';
    }
    else {
        int year = readInt("年份：", 1, 9999);
        int month = readInt("月份：", 1, 12);
        auto stats = m_borrowService.getMonthlyStatistics(year, month);
        std::cout << "借出：" << stats.totalBorrowed
            << " | 归还：" << stats.totalReturned
            << " | 逾期：" << stats.overdueCount
            << " | 罚金：" << std::fixed << std::setprecision(2) << stats.totalFines << '\n';
    }
    pauseAndWait();
}

void InteractiveShell::performAdminBorrowOverdue() {
    auto overdue = m_borrowService.getOverdueRecords();
    printBorrowRecords(overdue, "逾期借阅列表");
    pauseAndWait();
}

/* --- 预约管理 --- */

void InteractiveShell::performAdminReservationQueue() {
    std::string isbn = readLine("请输入 ISBN：");
    auto queue = m_reservationSystem.getReservationQueue(isbn);
    if (queue.empty()) {
        std::cout << "暂无预约。\n";
    }
    else {
        std::cout << "预约队列：\n";
        int pos = 0;
        for (const auto& readerId : queue) {
            std::cout << ++pos << ". " << readerId << '\n';
        }
    }
    pauseAndWait();
}

void InteractiveShell::performAdminReservationProcess() {
    m_reservationSystem.processAvailableReservations();
    std::cout << "已处理可发放预约，并通知下一位读者。\n";
    pauseAndWait();
}

void InteractiveShell::performAdminReservationCancel() {
    std::string readerId = readLine("读者ID：");
    std::string isbn = readLine("ISBN：");
    if (m_reservationSystem.cancelReservation(readerId, isbn)) {
        std::cout << "已取消预约。\n";
    }
    else {
        std::cout << "取消失败，请确认读者和 ISBN 是否存在预约。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminReservationPriority() {
    std::string readerId = readLine("读者ID：");
    std::string isbn = readLine("ISBN：");
    std::cout << "请选择优先级：0=Low 1=Normal 2=High 3=Critical\n";
    int level = readInt("输入选项：", 0, 3);
    if (m_reservationSystem.setReservationPriority(readerId, isbn, static_cast<PriorityLevel>(level))) {
        std::cout << "优先级已调整。\n";
    }
    else {
        std::cout << "调整失败，请确认预约存在且未完成。\n";
    }
    pauseAndWait();
}

/* --- 通知中心 --- */

void InteractiveShell::performAdminSendDueReminder() {
    std::string readerId = readLine("读者ID：");
    if (m_notificationCenter.sendDueDateReminder(readerId)) {
        std::cout << "已发送到期提醒。\n";
    }
    else {
        std::cout << "未找到合适的到期记录或读者不可用。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminSendCustomNotification() {
    std::string readerId = readLine("读者ID（留空则批量发送）：");
    std::string message = readLine("通知内容：");
    if (message.empty()) {
        std::cout << "消息不能为空。\n";
    }
    else if (!readerId.empty()) {
        if (m_notificationCenter.sendCustomNotification(readerId, message)) {
            std::cout << "发送成功。\n";
        }
        else {
            std::cout << "发送失败。\n";
        }
    }
    else {
        std::string ids = readLine("请输入多个读者ID，逗号分隔：");
        auto list = StringUtil::split(ids, ',');
        for (auto& id : list) id = StringUtil::trim(id);
        if (m_notificationCenter.sendBulkNotification(list, message)) {
            std::cout << "批量通知已发送。\n";
        }
        else {
            std::cout << "发送失败。\n";
        }
    }
    pauseAndWait();
}

/* --- 信用体系 --- */

void InteractiveShell::performAdminCreditView() {
    std::string readerId = readLine("读者ID：");
    if (!m_readerService.checkReaderIdExists(readerId)) {
        std::cout << "读者不存在。\n";
    }
    else {
        int score = m_creditSystem.calculateCreditScore(readerId);
        auto level = m_creditSystem.getCreditLevel(readerId);
        std::cout << "信用分：" << score << " | 等级：" << static_cast<int>(level) << '\n';
        std::cout << "最大可借：" << m_creditSystem.getMaxBooksAllowed(readerId)
            << " | 最大借阅天数：" << m_creditSystem.getBorrowDaysAllowed(readerId)
            << " | 罚金折扣：" << m_creditSystem.getFineDiscountRate(readerId) * 100 << "%\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminCreditAdjust() {
    std::string readerId = readLine("读者ID：");
    if (!m_readerService.checkReaderIdExists(readerId)) {
        std::cout << "读者不存在。\n";
        pauseAndWait();
        return;
    }
    int delta = readInt("积分调整（正数加分、负数扣分）：", -500, 500);
    std::string reason = readLine("原因：");
    if (delta > 0) {
        m_creditSystem.addCreditPoints(readerId, delta, reason);
        std::cout << "已加分。\n";
    }
    else if (delta < 0) {
        m_creditSystem.deductCreditPoints(readerId, -delta, reason);
        std::cout << "已扣分。\n";
    }
    else {
        std::cout << "未调整积分。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminCreditSuspend() {
    std::string readerId = readLine("读者ID：");
    int days = readInt("暂停天数：", 1, 365);
    if (m_creditSystem.suspendAccount(readerId, days)) {
        std::cout << "账户已暂停。\n";
    }
    else {
        std::cout << "操作失败，请确认读者存在。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminCreditRestore() {
    std::string readerId = readLine("读者ID：");
    if (m_creditSystem.restoreAccount(readerId)) {
        std::cout << "账户已恢复。\n";
    }
    else {
        std::cout << "操作失败。\n";
    }
    pauseAndWait();
}

/* --- 数据洞察 / 推荐 --- */

void InteractiveShell::performAdminGenerateReport() {
    Date today = DateUtil::today();
    auto report = m_reportGenerator.generateDailyReport(today);
    std::cout << report << '\n';

    std::string save = readLine("是否导出到文件？(y/n)：");
    if (!save.empty() && (save[0] == 'y' || save[0] == 'Y')) {
        std::string path = readLine("请输入文件路径（默认 daily_report.txt）：");
        if (path.empty()) path = "daily_report.txt";
        if (m_reportGenerator.exportReportToFile(report, path)) {
            std::cout << "已导出到 " << path << '\n';
        }
        else {
            std::cout << "导出失败。\n";
        }
    }
    pauseAndWait();
}

void InteractiveShell::performAdminMonthlyReport() {
    Date today = DateUtil::today();
    auto report = m_reportGenerator.generateMonthlyReport(today.year, today.month);
    std::cout << report << '\n';
    pauseAndWait();
}

void InteractiveShell::performAdminBorrowTrend() {
    Date today = DateUtil::today();
    DateRange range{ DateUtil::addDays(today, -30), today };
    auto trend = m_dataAnalyzer.analyzeBorrowTrend(range);
    std::cout << "近 30 天借阅趋势：\n";
    for (const auto& [date, count] : trend.dailyCounts) {
        std::cout << DateUtil::toString(date) << " -> " << count << '\n';
    }
    pauseAndWait();
}

void InteractiveShell::performAdminPredictPopular() {
    auto forecast = m_dataAnalyzer.predictPopularBooks(15);
    std::cout << "未来热门图书预测（ISBN 列表）：\n";
    for (const auto& isbn : forecast) {
        std::cout << "- " << isbn << '\n';
    }
    pauseAndWait();
}

/* --- 社区管理 --- */

void InteractiveShell::performAdminCommunityStats() {
    auto stats = m_communityManager.getCommunityStats();
    std::cout << "讨论串：" << stats.threadCount
        << "，帖子：" << stats.postCount
        << "，评论：" << stats.commentCount
        << "，活跃读者：" << stats.activeReaderCount << '\n';
    pauseAndWait();
}

void InteractiveShell::performAdminCreateThread() {
    std::string isbn = readLine("关联图书 ISBN：");
    std::string title = readLine("讨论串标题：");
    std::string creator = readLine("创建者读者ID：");
    auto threadIdOpt = m_communityManager.createThread(isbn, title, creator);
    if (threadIdOpt) {
        std::cout << "创建成功，讨论串ID：" << threadIdOpt.value() << '\n';
    }
    else {
        std::cout << "创建失败，请检查 ISBN 或读者是否存在/激活。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminListThreads() {
    std::string isbn = readLine("请输入 ISBN：");
    auto threads = m_communityManager.listThreadsByBook(isbn);
    if (threads.empty()) {
        std::cout << "暂无讨论串。\n";
    }
    else {
        for (const auto& thread : threads) {
            std::cout << "- ID：" << thread.threadId
                << " | 标题：" << thread.title
                << " | 创建者：" << thread.creatorId
                << " | 帖子数：" << thread.postIds.size() << '\n';
        }
    }
    pauseAndWait();
}

void InteractiveShell::performAdminCloseThread() {
    std::string threadId = readLine("请输入讨论串ID：");
    if (m_communityManager.closeThread(threadId)) {
        std::cout << "讨论串已关闭。\n";
    }
    else {
        std::cout << "关闭失败，可能 ID 不存在。\n";
    }
    pauseAndWait();
}

/* --- 多终端同步 --- */

void InteractiveShell::performAdminRegisterDevice() {
    DeviceInfo device;
    device.deviceId = readLine("设备ID：");
    device.deviceName = readLine("设备名称：");
    device.location = readLine("部署地点：");
    device.lastSyncDate = DateUtil::today();
    device.status = SyncStatus::Idle;

    if (device.deviceId.empty() || device.deviceName.empty()) {
        std::cout << "设备ID或名称不能为空。\n";
    }
    else if (m_multiSync.registerDevice(device)) {
        std::cout << "设备注册成功。\n";
    }
    else {
        std::cout << "注册失败，可能该设备已存在。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminListDevices() {
    auto devices = m_multiSync.listDevices();
    if (devices.empty()) {
        std::cout << "暂无注册设备。\n";
    }
    else {
        for (const auto& device : devices) {
            std::cout << "- " << device.deviceId
                << " | 名称：" << device.deviceName
                << " | 地点：" << device.location
                << " | 上次同步：" << DateUtil::toString(device.lastSyncDate)
                << " | 状态：" << static_cast<int>(device.status) << '\n';
        }
    }
    pauseAndWait();
}

void InteractiveShell::performAdminScheduleSync() {
    std::string deviceId = readLine("设备ID：");
    int days = readInt("距今多少天后同步（0-365）：", 0, 365);
    Date target = DateUtil::addDays(DateUtil::today(), days);
    if (m_multiSync.scheduleSync(deviceId, target)) {
        std::cout << "已安排同步至 " << DateUtil::toString(target) << '\n';
    }
    else {
        std::cout << "安排失败，设备可能未注册。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminCancelSync() {
    std::string deviceId = readLine("设备ID：");
    if (m_multiSync.cancelScheduledSync(deviceId)) {
        std::cout << "已取消计划同步。\n";
    }
    else {
        std::cout << "取消失败，该设备可能未安排同步或未注册。\n";
    }
    pauseAndWait();
}

/* --- 数据持久化 --- */

void InteractiveShell::performAdminSaveData() {
    if (m_fileManager.saveAllData()) {
        std::cout << "保存成功。\n";
    }
    else {
        std::cout << "保存失败。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminLoadData() {
    if (m_fileManager.loadAllData()) {
        std::cout << "载入成功。\n";
    }
    else {
        std::cout << "载入失败，请检查文件。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminExportCSV() {
    std::string filename = readLine("请输入 CSV 文件名（默认 library_export.csv）：");
    if (filename.empty()) filename = "library_export.csv";
    if (m_fileManager.exportToCSV(filename)) {
        std::cout << "已导出到 " << filename << '\n';
    }
    else {
        std::cout << "导出失败。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminExportJSON() {
    std::string filename = readLine("请输入 JSON 文件名（默认 library_export.json）：");
    if (filename.empty()) filename = "library_export.json";
    if (m_fileManager.exportToJSON(filename)) {
        std::cout << "已导出到 " << filename << '\n';
    }
    else {
        std::cout << "导出失败。\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminBackup() {
    if (m_fileManager.createAutoBackup()) {
        std::cout << "备份创建成功。\n";
    }
    else {
        std::cout << "备份失败。\n";
    }
    pauseAndWait();
}

/* --- 安全与审计 --- */

void InteractiveShell::performAdminAuditLogs() {
    int days = readInt("查询最近多少天的审计日志？(1-30)：", 1, 30);
    Date end = DateUtil::today();
    Date start = DateUtil::addDays(end, -days);

    std::cout << "请选择最低日志级别：\n"
        << " 0. Info\n"
        << " 1. Warning\n"
        << " 2. Critical\n";
    int level = readInt("输入选项：", 0, 2);
    AuditSeverity severity = static_cast<AuditSeverity>(level);

    auto logs = m_securityManager.queryAuditLogs(start, end, severity);
    std::cout << "\n==== 审计日志（" << logs.size() << " 条）====\n";
    for (const auto& entry : logs) {
        std::cout << entry.entryId << " | "
            << DateUtil::toString(entry.timestamp)
            << " | 操作者：" << entry.actorId
            << " | 动作：" << entry.action
            << " | 等级：" << severityToString(entry.severity)
            << " | " << entry.details << '\n';
    }
    if (logs.empty()) {
        std::cout << "（暂无符合条件的日志）\n";
    }
    pauseAndWait();
}

void InteractiveShell::performAdminRiskAssessment() {
    m_securityManager.performRiskAssessment();
    auto alerts = m_securityManager.listAlerts(true);

    std::cout << "\n==== 当前告警（" << alerts.size() << " 条）====\n";
    for (const auto& alert : alerts) {
        std::cout << alert.alertId << " | "
            << DateUtil::toString(alert.triggeredAt)
            << " | 读者：" << alert.readerId
            << " | 等级：" << severityToString(alert.severity)
            << " | " << alert.description
            << " | 已确认：" << (alert.acknowledged ? "是" : "否") << '\n';
    }
    if (alerts.empty()) {
        std::cout << "暂无告警。\n";
    }
    else {
        std::string ack = readLine("是否将所有未确认告警标记为已确认？(y/n)：");
        if (!ack.empty() && (ack[0] == 'y' || ack[0] == 'Y')) {
            for (const auto& alert : alerts) {
                if (!alert.acknowledged) {
                    m_securityManager.acknowledgeAlert(alert.alertId);
                }
            }
            std::cout << "已确认所有告警。\n";
        }
    }
    pauseAndWait();
}

/* --- 工具函数 --- */

// 暂停并等待用户回车
void InteractiveShell::pauseAndWait() const {
    std::cout << "\n按回车继续..." << std::flush;
    std::string dummy;
    std::getline(std::cin, dummy);
}

// 辅助读取整数：循环直到输入合法
int InteractiveShell::readInt(const std::string& prompt, int min, int max) const {
    while (true) {
        std::cout << prompt << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cin.clear();
            continue;
        }
        std::stringstream ss(line);
        int value = 0;
        if ((ss >> value)) {
            ss >> std::ws;
            if (ss.eof() && value >= min && value <= max) {
                return value;
            }
        }
        std::cout << "输入无效，请输入范围 [" << min << ", " << max << "] 内的整数。\n";
    }
}

// 辅助读取字符串：去除首尾空格
std::string InteractiveShell::readLine(const std::string& prompt) const {
    std::cout << prompt << std::flush;
    std::string line;
    std::getline(std::cin, line);
    return StringUtil::trim(line);
}

// 打印分隔符
void InteractiveShell::printSeparator() const {
    std::cout << "\n----------------------------------------\n";
}

