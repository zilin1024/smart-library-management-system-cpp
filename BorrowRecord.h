#ifndef BORROW_RECORD_H
#define BORROW_RECORD_H

#include "DateUtil.h"
#include <optional>
#include <string>

/**
 * @class BorrowRecord
 * @brief 借阅记录核心实体，用于跟踪每一次图书借阅与归还的全过程。
 */
class BorrowRecord {
public:
    BorrowRecord();
    BorrowRecord(std::string recordId,
        std::string readerId,
        std::string isbn,
        const Date& borrowDate,
        const Date& dueDate,
        int borrowHour = 0);

    // === 基本信息访问 ===
    [[nodiscard]] const std::string& getRecordId() const noexcept;
    void setRecordId(const std::string& recordId);

    [[nodiscard]] const std::string& getReaderId() const noexcept;
    void setReaderId(const std::string& readerId);

    [[nodiscard]] const std::string& getISBN() const noexcept;
    void setISBN(const std::string& isbn);

    [[nodiscard]] const Date& getBorrowDate() const noexcept;
    void setBorrowDate(const Date& date);

    [[nodiscard]] const Date& getDueDate() const noexcept;
    void setDueDate(const Date& date);

    [[nodiscard]] const std::optional<Date>& getReturnDate() const noexcept;
    void setReturnDate(const Date& date);
    void clearReturnDate();

    [[nodiscard]] int getBorrowHour() const noexcept;
    void setBorrowHour(int hour);

    // === 续借与状态 ===
    [[nodiscard]] int getRenewCount() const noexcept;
    void incrementRenewCount();

    [[nodiscard]] bool isReturned() const noexcept;
    [[nodiscard]] bool isOverdue(const Date& onDate) const noexcept;
    [[nodiscard]] int getBorrowDuration() const noexcept;
    [[nodiscard]] int getOverdueDays(const Date& onDate) const noexcept;

    // === 罚金与统计 ===
    [[nodiscard]] double getFine() const noexcept;
    void setFine(double fine);

    // === 序列化支持 ===
    [[nodiscard]] std::string serialize(char delimiter = '|') const;
    static BorrowRecord deserialize(const std::string& line, char delimiter = '|');

private:
    std::string m_recordId;    // 记录ID（唯一标识）
    std::string m_readerId;    // 读者ID（关联Reader）
    std::string m_isbn;        // 图书ISBN（关联Book）

    Date m_borrowDate;           // 借阅日期（起始点）
    Date m_dueDate;              // 应还日期（截止点）
    std::optional<Date> m_returnDate;  // 实际归还日期（可空）
    int m_borrowHour{ 0 };         // 借阅小时（用于精确到小时的计算）

    int m_renewCount{ 0 };          // 续借次数（限制续借上限）
    double m_fine{ 0.0 };           // 罚金金额（逾期产生）
};

#endif // BORROW_RECORD_H
