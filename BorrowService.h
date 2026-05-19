#ifndef BORROW_SERVICE_H
#define BORROW_SERVICE_H

#include "Book.h"
#include "BorrowRecord.h"
#include "LibraryManager.h"
#include "Reader.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @enum BorrowStatus
 * @brief 借书操作的状态码。
 */
enum class BorrowStatus {
    Success,
    ReaderNotFound,
    BookNotFound,
    BookOutOfStock,
    LimitExceeded,
    AlreadyBorrowed,
    SuspendedAccount,
    ReservationRequired,
    Failed
};

/**
 * @enum ReturnStatus
 * @brief 还书操作的状态码。
 */
enum class ReturnStatus {
    Success,
    ReaderNotFound,
    BookNotFound,
    RecordNotFound,
    AlreadyReturned,
    Failed
};

/**
 * @struct BorrowResult
 * @brief 借书操作的返回结构。
 */
struct BorrowResult {
    BorrowStatus status{ BorrowStatus::Failed };
    std::string message;
    std::shared_ptr<BorrowRecord> record;// 使用shared_ptr管理资源
};

/**
 * @struct ReturnResult
 * @brief 还书操作的返回结构。
 */
struct ReturnResult {
    ReturnStatus status{ ReturnStatus::Failed };
    std::string message;
    double fine{ 0.0 };
    std::shared_ptr<BorrowRecord> record;// 使用shared_ptr管理资源
};

/**
 * @struct BorrowStatistics
 * @brief 借阅统计数据。
 */
struct BorrowStatistics {
    int totalBorrowed{ 0 };
    int totalReturned{ 0 };
    int overdueCount{ 0 };
    double totalFines{ 0.0 };
};

/**
 * @class BorrowService
 * @brief 借阅业务服务层，实现借书、还书、续借与统计功能。
 */
class BorrowService {
public:
    explicit BorrowService(LibraryManager& manager, int defaultBorrowDays = 14, int maxRenewals = 2);

    // ===== 1. 借书操作 =====
    BorrowResult borrowBook(const std::string& readerId, const std::string& isbn);
    bool checkBorrowEligibility(const std::string& readerId, const std::string& isbn);
    bool reserveBook(const std::string& readerId, const std::string& isbn);

    // ===== 2. 还书操作 =====
    ReturnResult returnBook(const std::string& readerId, const std::string& isbn);
    double calculateOverdueFine(const BorrowRecord& record) const;
    bool renewBook(const std::string& readerId, const std::string& isbn);

    // ===== 3. 借阅记录查询 =====
    std::vector<BorrowRecord> getReaderBorrowHistory(const std::string& readerId);
    std::vector<BorrowRecord> getBookBorrowHistory(const std::string& isbn);
    std::vector<BorrowRecord> getCurrentBorrows(const std::string& readerId);
    std::vector<BorrowRecord> getOverdueRecords();
    std::vector<BorrowRecord> getRecordsByDateRange(Date start, Date end);
    std::vector<BorrowRecord> getAllBorrowRecords() const;

    // ===== 4. 统计功能 =====
    BorrowStatistics getDailyStatistics(Date date);
    BorrowStatistics getMonthlyStatistics(int year, int month);
    std::map<std::string, int> getPopularBooks(int limit = 20);

    // ===== 5. 配置 =====
    void setFinePerDay(double fine);
    void setDefaultBorrowDays(int days);
    void setMaxRenewals(int count);

private:
    LibraryManager& m_library; // 通过构造函数注入
    int m_defaultBorrowDays;
    int m_maxRenewals;
    double m_finePerDay{ 0.5 };

    std::optional<BorrowRecord*> findActiveBorrowRecord(const std::string& readerId, const std::string& isbn) const;
    bool ensureReaderEligibility(const Reader& reader);
    void releaseSuspensionIfExpired(Reader& reader);
    BorrowResult makeBorrowResult(BorrowStatus status, const std::string& message,
        BorrowRecord* recordPtr = nullptr) const;
    ReturnResult makeReturnResult(ReturnStatus status, const std::string& message,
        double fine = 0.0, BorrowRecord* recordPtr = nullptr) const;
};

#endif // BORROW_SERVICE_H
