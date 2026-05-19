#ifndef RESERVATION_SYSTEM_H
#define RESERVATION_SYSTEM_H

#include "BookService.h"
#include "BorrowService.h"
#include "ReaderService.h"

#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @enum PriorityLevel
 * @brief 预约优先级枚举。
 */
enum class PriorityLevel {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @enum ReservationStatus
 * @brief 预约状态。
 */
enum class ReservationStatus {
    Active,
    Fulfilled,
    Cancelled,
    Expired
};

/**
 * @struct ReservationUpdate
 * @brief 预约修改信息。
 */
struct ReservationUpdate {
    std::optional<Date> newPickupDate;
    std::optional<PriorityLevel> newPriority;
};

/**
 * @struct ReservationRecord
 * @brief 预约记录实体。
 */
struct ReservationRecord {
    std::string reservationId;
    std::string readerId;
    std::string isbn;
    Date requestDate;
    Date pickupDeadline;
    PriorityLevel priority{ PriorityLevel::Normal };
    ReservationStatus status{ ReservationStatus::Active };
};

/**
 * @struct ReservationStats
 * @brief 预约统计信息。
 */
struct ReservationStats {
    int totalReservations{ 0 };
    int activeReservations{ 0 };
    int fulfilledReservations{ 0 };
    int expiredReservations{ 0 };
};

/**
 * @struct ReservationResult
 * @brief 预约操作返回结果。
 */
struct ReservationResult {
    bool success{ false };
    std::string message;
    std::string reservationId;
    int positionInQueue{ -1 };
};

/**
 * @class ReservationSystem
 * @brief 高级功能层预约管理系统，实现图书预约排队与自动处理。
 */
class ReservationSystem {
public:
    ReservationSystem(BookService& bookService,
        ReaderService& readerService,
        BorrowService& borrowService,
        int defaultHoldDays = 3);

    // === 预约操作 ===
    ReservationResult reserveBook(const std::string& readerId, const std::string& isbn);
    bool cancelReservation(const std::string& readerId, const std::string& isbn);
    bool modifyReservation(const std::string& reservationId, const ReservationUpdate& update);

    // === 队列管理 ===
    std::vector<std::string> getReservationQueue(const std::string& isbn) const;
    int getQueuePosition(const std::string& readerId, const std::string& isbn) const;
    bool bumpInQueue(const std::string& reservationId, int newPosition);

    // === 自动处理 ===
    void processAvailableReservations();
    void expireOldReservations();
    void notifyNextInQueue(const std::string& isbn);

    // === 统计与分析 ===
    ReservationStats getReservationStatistics() const;
    std::map<std::string, int> getMostReservedBooks(int limit = 10) const;
    double getReservationFulfillmentRate() const;

    // === 高级功能 ===
    bool setReservationPriority(const std::string& readerId, const std::string& isbn, PriorityLevel priority);
    bool createGroupReservation(const std::vector<std::string>& readerIds, const std::string& isbn);
    bool transferReservation(const std::string& fromReaderId, const std::string& toReaderId, const std::string& isbn);

private:
    BookService& m_bookService;
    ReaderService& m_readerService;
    BorrowService& m_borrowService;
    int m_defaultHoldDays;
    long long m_sequence{ 0 };

    std::unordered_map<std::string, std::vector<ReservationRecord>> m_reservationsByISBN;
    std::unordered_map<std::string, std::pair<std::string, std::size_t>> m_indexById;

    std::string generateReservationId();
    void rebuildIndex(const std::string& isbn);
    std::optional<std::pair<std::string, std::size_t>> locateReservation(const std::string& reservationId);
    bool ensureReservationPrerequisites(const std::string& readerId, const std::string& isbn, std::string& error) const;
    void sortQueue(std::vector<ReservationRecord>& queue);
};

#endif // RESERVATION_SYSTEM_H
