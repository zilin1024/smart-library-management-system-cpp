
#include "NotificationCenter.h"
#include "DateUtil.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

// === 构造与基础设施 =======================================================

// 构造函数：初始化依赖的服务（读者、借阅、图书服务）及计数器
NotificationCenter::NotificationCenter(ReaderService& readerService,
    BorrowService& borrowService,
    BookService& bookService)
    : m_readerService(readerService),
    m_borrowService(borrowService),
    m_bookService(bookService),
    m_notifications(),
    m_stats(),
    m_sequence(0) {}

// 核心内部函数：构建并存储通知
// 负责校验读者状态（必须存在且激活），生成唯一ID，更新统计数据
bool NotificationCenter::appendNotification(const std::string& readerId, const std::string& message) {
    if (readerId.empty() || message.empty()) {
        return false;
    }

    const Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr || !reader->isActive() || reader->isSuspended()) {
        return false; // 读者无效或被停用时不发送
    }

    Notification notification;
    notification.id = generateNotificationId();
    notification.readerId = readerId;
    notification.message = message;
    notification.sentDate = DateUtil::today();
    notification.read = false;

    m_notifications[readerId].push_back(notification);
    ++m_stats.sentCount;
    ++m_stats.deliveredCount; // 发送即送达
    return true;
}

// 生成唯一的通知ID (格式: NTF-Sequence)
std::string NotificationCenter::generateNotificationId() {
    ++m_sequence;
    std::ostringstream oss;
    oss << "NTF-" << m_sequence;
    return oss.str();
}

// 根据图书分类筛选目标读者群
// 逻辑：遍历所有活跃读者，分析其阅读历史偏好，若匹配分类则加入列表
// 若 category 为空，则返回所有活跃读者
std::vector<std::string> NotificationCenter::resolveReadersByCategory(const std::string& category) const {
    std::vector<std::string> readerIds;
    auto candidates = m_readerService.getReadersByBorrowCount(0); // 获取所有读者（参数0表示不限借阅数）
    for (const auto& reader : candidates) {
        if (!reader.isActive() || reader.isSuspended()) {
            continue;
        }
        if (category.empty()) {
            readerIds.push_back(reader.getReaderId());
            continue;
        }
        const auto preferences = inferReaderCategories(reader);
        if (preferences.find(category) != preferences.end()) {
            readerIds.push_back(reader.getReaderId());
        }
    }
    return readerIds;
}

// 辅助函数：获取某读者即将到期（最近）的一条借阅记录
std::optional<BorrowRecord> NotificationCenter::getNearestDueRecord(const std::string& readerId) const {
    auto current = m_borrowService.getCurrentBorrows(readerId);
    if (current.empty()) {
        return std::nullopt;
    }

    // 按到期时间升序排序
    std::sort(current.begin(), current.end(), [](const BorrowRecord& lhs, const BorrowRecord& rhs) {
        return lhs.getDueDate() < rhs.getDueDate();
        });

    return current.front();
}

// === 借阅提醒 =============================================================

// 发送即将到期提醒
// 策略：仅在到期前 0-3 天内发送；若已逾期则不发送（交由逾期警告处理）
bool NotificationCenter::sendDueDateReminder(const std::string& readerId) {
    auto nearestOpt = getNearestDueRecord(readerId);
    if (!nearestOpt.has_value()) {
        return false;
    }

    const BorrowRecord& record = nearestOpt.value();
    const Date today = DateUtil::today();
    const int daysLeft = DateUtil::daysBetween(today, record.getDueDate());

    if (daysLeft < 0) {
        return false; // 已逾期，转由逾期提醒处理。
    }
    if (daysLeft > 3) {
        return false; // 距离到期还早，不打扰读者。
    }

    std::ostringstream oss;
    oss << "亲爱的读者，您借阅的图书（ISBN：" << record.getISBN()
        << "）将于 " << DateUtil::toString(record.getDueDate())
        << " 到期，请及时归还或办理续借。";

    return appendNotification(readerId, oss.str());
}

// 发送逾期警告
// 策略：遍历读者当前所有借阅，只要有一本逾期就发送警告
bool NotificationCenter::sendOverdueAlert(const std::string& readerId) {
    const auto overdueRecords = m_borrowService.getCurrentBorrows(readerId);
    const Date today = DateUtil::today();
    bool sent = false;

    for (const auto& record : overdueRecords) {
        if (record.isOverdue(today)) {
            std::ostringstream oss;
            oss << "警告：您借阅的图书（ISBN：" << record.getISBN()
                << "）已于 " << DateUtil::toString(record.getDueDate())
                << " 逾期，请尽快归还。";
            sent = appendNotification(readerId, oss.str()) || sent;
        }
    }
    return sent;
}

// 发送续借建议
// 策略：当书快到期（剩余0或1天）时，提示可以续借
bool NotificationCenter::sendRenewalReminder(const std::string& readerId) {
    auto nearestOpt = getNearestDueRecord(readerId);
    if (!nearestOpt.has_value()) {
        return false;
    }

    const BorrowRecord& record = nearestOpt.value();
    const Date today = DateUtil::today();
    const int daysLeft = DateUtil::daysBetween(today, record.getDueDate());

    if (daysLeft < 0 || daysLeft > 1) {
        return false;
    }

    std::ostringstream oss;
    oss << "您借阅的图书（ISBN：" << record.getISBN()
        << "）即将到期。如果需要延长借阅时间，请及时办理续借。";
    return appendNotification(readerId, oss.str());
}

// === 预约通知 =============================================================

// 发送预约到货通知
bool NotificationCenter::sendReservationAvailable(const std::string& readerId, const std::string& isbn) {
    std::ostringstream oss;
    auto books = m_bookService.searchByISBN(isbn);
    if (!books.empty()) {
        oss << "预约通知：您预约的《" << books.front().getTitle()
            << "》（ISBN：" << isbn << "）现已可借，请在指定时间内前往取书。";
    }
    else {
        oss << "预约通知：您预约的图书（ISBN：" << isbn << "）现已可借，请在指定时间内前往取书。";
    }
    return appendNotification(readerId, oss.str());
}

// 发送预约即将过期通知
bool NotificationCenter::sendReservationExpiring(const std::string& readerId, const std::string& isbn) {
    std::ostringstream oss;
    auto books = m_bookService.searchByISBN(isbn);
    if (!books.empty()) {
        oss << "提醒：您预约的《" << books.front().getTitle()
            << "》（ISBN：" << isbn << "）即将过期，请尽快前往取书。";
    }
    else {
        oss << "提醒：您预约的图书（ISBN：" << isbn << "）即将过期，请尽快前往取书。";
    }
    return appendNotification(readerId, oss.str());
}

// === 新书通知 =============================================================

// 发送新书上架通知（针对特定读者）
bool NotificationCenter::sendNewBookNotification(const std::string& readerId,
    const std::vector<std::string>& newBooks) {
    if (newBooks.empty()) {
        return false;
    }

    std::ostringstream oss;
    oss << "您好！图书馆新上架了以下图书：\n";
    for (const auto& isbn : newBooks) {
        auto books = m_bookService.searchByISBN(isbn);
        if (!books.empty()) {
            oss << " - 《" << books.front().getTitle() << "》 (ISBN：" << isbn << ")\n";
        }
        else {
            oss << " - ISBN：" << isbn << '\n';
        }
    }
    return appendNotification(readerId, oss.str());
}

// 根据类别推送新书通知
// 逻辑：找到对该类别感兴趣的所有读者，批量发送通知
bool NotificationCenter::sendCategoryUpdate(const std::string& category,
    const std::vector<std::string>& newBooks) {
    const auto targetReaders = resolveReadersByCategory(category);
    if (targetReaders.empty()) {
        return false;
    }

    std::ostringstream oss;
    oss << "您关注的类别“" << category << "”有以下新书上架：\n";
    for (const auto& isbn : newBooks) {
        auto books = m_bookService.searchByISBN(isbn);
        if (!books.empty()) {
            oss << " - 《" << books.front().getTitle() << "》 (ISBN：" << isbn << ")\n";
        }
        else {
            oss << " - ISBN：" << isbn << '\n';
        }
    }

    bool sent = false;
    for (const auto& readerId : targetReaders) {
        sent = appendNotification(readerId, oss.str()) || sent;
    }
    return sent;
}

// === 活动通知 =============================================================

// 发送图书馆活动通知
// 逻辑：分析活动描述中的关键词，匹配感兴趣的读者群体进行推送
bool NotificationCenter::sendLibraryEventNotification(const LibraryEvent& event) {
    const std::string merged = event.title + " " + event.description;
    auto matchedCategories = extractCategoriesFromText(merged);

    std::vector<std::string> recipients;
    if (matchedCategories.empty()) {
        recipients = resolveReadersByCategory(""); // 无特定分类则全员推送
    }
    else {
        std::unordered_set<std::string> unionReaders;
        for (const auto& cat : matchedCategories) {
            auto partial = resolveReadersByCategory(cat);
            unionReaders.insert(partial.begin(), partial.end());
        }
        recipients.assign(unionReaders.begin(), unionReaders.end());
    }

    if (recipients.empty()) {
        return false;
    }

    std::ostringstream oss;
    oss << "图书馆活动通知：\n"
        << "活动名称：" << event.title << "\n"
        << "活动时间：" << DateUtil::toString(event.date) << "\n"
        << "活动详情：" << event.description;

    bool sent = false;
    for (const auto& readerId : recipients) {
        sent = appendNotification(readerId, oss.str()) || sent;
    }
    return sent;
}

// 发送促销/优惠通知
// 逻辑同活动通知，基于文本内容匹配受众
bool NotificationCenter::sendPromotionNotification(const Promotion& promotion) {
    const std::string merged = promotion.title + " " + promotion.details;
    auto matchedCategories = extractCategoriesFromText(merged);

    std::vector<std::string> recipients;
    if (matchedCategories.empty()) {
        recipients = resolveReadersByCategory("");
    }
    else {
        std::unordered_set<std::string> unionReaders;
        for (const auto& cat : matchedCategories) {
            auto partial = resolveReadersByCategory(cat);
            unionReaders.insert(partial.begin(), partial.end());
        }
        recipients.assign(unionReaders.begin(), unionReaders.end());
    }

    if (recipients.empty()) {
        return false;
    }

    std::ostringstream oss;
    oss << "优惠活动提醒：" << promotion.title << "\n"
        << promotion.details << "\n"
        << "有效期：" << DateUtil::toString(promotion.startDate)
        << " 至 " << DateUtil::toString(promotion.endDate);

    bool sent = false;
    for (const auto& readerId : recipients) {
        sent = appendNotification(readerId, oss.str()) || sent;
    }
    return sent;
}

// === 自定义通知 ===========================================================

// 发送单条自定义通知
bool NotificationCenter::sendCustomNotification(const std::string& readerId, const std::string& message) {
    return appendNotification(readerId, message);
}

// 批量发送通知
bool NotificationCenter::sendBulkNotification(const std::vector<std::string>& readerIds,
    const std::string& message) {
    if (readerIds.empty() || message.empty()) {
        return false;
    }

    bool sent = false;
    for (const auto& readerId : readerIds) {
        sent = appendNotification(readerId, message) || sent;
    }
    return sent;
}

// === 通知管理 =============================================================

// 获取指定读者的所有未读通知
std::vector<Notification> NotificationCenter::getUnreadNotifications(const std::string& readerId) {
    std::vector<Notification> unread;
    auto it = m_notifications.find(readerId);
    if (it == m_notifications.end()) {
        return unread;
    }

    for (const auto& notification : it->second) {
        if (!notification.read) {
            unread.push_back(notification);
        }
    }
    return unread;
}

// 标记通知为已读，并更新统计数据
bool NotificationCenter::markAsRead(const std::string& notificationId) {
    for (auto& [readerId, notifications] : m_notifications) {
        for (auto& notification : notifications) {
            if (notification.id == notificationId && !notification.read) {
                notification.read = true;
                ++m_stats.readCount;
                return true;
            }
        }
    }
    return false;
}

NotificationStats NotificationCenter::getNotificationStatistics() {
    return m_stats;
}

// === 新增的辅助函数 =======================================================

// 推断读者偏好类别
// 综合分析：借阅历史 + 当前借阅 + 预约记录 + 本地数据
std::set<std::string> NotificationCenter::inferReaderCategories(const Reader& reader) const {
    std::set<std::string> categories;

    const auto& history = reader.getBorrowHistory();
    const auto& current = reader.getCurrentBorrowedIsbns();
    const auto& reservations = reader.getReservations();

    for (const auto& isbn : history) {
        accumulateCategoriesFromISBN(isbn, categories);
    }
    for (const auto& isbn : current) {
        accumulateCategoriesFromISBN(isbn, categories);
    }
    for (const auto& isbn : reservations) {
        accumulateCategoriesFromISBN(isbn, categories);
    }

    // 再遍历借阅记录，防止 Reader 本地数据不完整
    auto records = m_borrowService.getReaderBorrowHistory(reader.getReaderId());
    for (const auto& record : records) {
        accumulateCategoriesFromISBN(record.getISBN(), categories);
    }

    return categories;
}

// 辅助：从单本图书的 ISBN 中提取分类和标签
void NotificationCenter::accumulateCategoriesFromISBN(const std::string& isbn,
    std::set<std::string>& categories) const {
    if (isbn.empty()) {
        return;
    }
    auto books = m_bookService.searchByISBN(isbn);
    if (books.empty()) {
        return;
    }

    const Book& book = books.front();
    if (!book.getCategory().empty()) {
        categories.insert(book.getCategory());
    }
    for (const auto& tag : book.getTags()) {
        if (!tag.empty()) {
            categories.insert(tag);
        }
    }
}

// 辅助：从文本（如活动描述）中提取匹配的图书分类
std::set<std::string> NotificationCenter::extractCategoriesFromText(const std::string& text) const {
    std::set<std::string> matched;
    if (text.empty()) {
        return matched;
    }

    const std::string lowered = StringUtil::toLower(text);
    auto categoryDist = m_bookService.getCategoryDistribution();
    for (const auto& item : categoryDist) {
        const std::string loweredCategory = StringUtil::toLower(item.first);
        if (!loweredCategory.empty() && lowered.find(loweredCategory) != std::string::npos) {
            matched.insert(item.first);
        }
    }
    return matched;
}
