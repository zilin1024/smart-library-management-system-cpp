#ifndef NOTIFICATION_CENTER_H
#define NOTIFICATION_CENTER_H

#include "BookService.h"
#include "BorrowService.h"
#include "ReaderService.h"
#include "StringUtil.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @struct LibraryEvent
 * @brief 图书馆活动信息结构体。
 */
struct LibraryEvent {
    std::string id;
    std::string title;
    Date date;
    std::string description;
};

/**
 * @struct Promotion
 * @brief 促销/推广信息结构体。
 */
struct Promotion {
    std::string id;
    std::string title;
    std::string details;
    Date startDate;
    Date endDate;
};

/**
 * @struct Notification
 * @brief 通知实体，记录通知内容与已读状态。
 */
struct Notification {
    std::string id;
    std::string readerId;
    std::string message;
    Date sentDate;
    bool read{ false };
};

/**
 * @struct NotificationStats
 * @brief 通知统计信息。
 */
struct NotificationStats {
    int sentCount{ 0 };
    int deliveredCount{ 0 };
    int readCount{ 0 };
};

/**
 * @class NotificationCenter
 * @brief 高级功能层通知中心，负责生成与派发各类通知。
 */
class NotificationCenter {
public:
    NotificationCenter(ReaderService& readerService,
        BorrowService& borrowService,
        BookService& bookService);

    // === 借阅提醒 ===
    bool sendDueDateReminder(const std::string& readerId);
    bool sendOverdueAlert(const std::string& readerId);
    bool sendRenewalReminder(const std::string& readerId);

    // === 预约通知 ===
    bool sendReservationAvailable(const std::string& readerId, const std::string& isbn);
    bool sendReservationExpiring(const std::string& readerId, const std::string& isbn);

    // === 新书通知 ===
    bool sendNewBookNotification(const std::string& readerId, const std::vector<std::string>& newBooks);
    bool sendCategoryUpdate(const std::string& category, const std::vector<std::string>& newBooks);

    // === 活动通知 ===
    bool sendLibraryEventNotification(const LibraryEvent& event);
    bool sendPromotionNotification(const Promotion& promotion);

    // === 自定义通知 ===
    bool sendCustomNotification(const std::string& readerId, const std::string& message);
    bool sendBulkNotification(const std::vector<std::string>& readerIds, const std::string& message);

    // === 通知管理 ===
    std::vector<Notification> getUnreadNotifications(const std::string& readerId);
    bool markAsRead(const std::string& notificationId);
    NotificationStats getNotificationStatistics();

private:
    ReaderService& m_readerService;
    BorrowService& m_borrowService;
    BookService& m_bookService;

    std::unordered_map<std::string, std::vector<Notification>> m_notifications;
    mutable NotificationStats m_stats{};
    long long m_sequence{ 0 };

    bool appendNotification(const std::string& readerId, const std::string& message);
    std::string generateNotificationId();
    std::vector<std::string> resolveReadersByCategory(const std::string& category) const;
    [[nodiscard]] std::optional<BorrowRecord> getNearestDueRecord(const std::string& readerId) const;

    // ===== 新增的辅助函数 =====
    std::set<std::string> inferReaderCategories(const Reader& reader) const;
    void accumulateCategoriesFromISBN(const std::string& isbn, std::set<std::string>& categories) const;
    std::set<std::string> extractCategoriesFromText(const std::string& text) const;
};

#endif // NOTIFICATION_CENTER_H
