#ifndef READER_H
#define READER_H

#include "DateUtil.h"
#include <optional>
#include <string>
#include <vector>

/**
 * @struct ContactInfo
 * @brief 读者联系方式结构体。
 */
struct ContactInfo {
    std::string phone;    // 联系电话
    std::string email;    // 电子邮箱
    std::string address;  // 联系地址
};

/**
 * @struct ReaderUpdate
 * @brief 读者更新信息结构体，用于 ReaderService 的信息修改接口。
 */
struct ReaderUpdate {
    std::optional<std::string> name;       // 可选姓名更新
    std::optional<ContactInfo> contact;    // 可选联系信息更新
    std::optional<int> maxBorrow;          // 可选最大借阅数更新
    std::optional<int> age;                // 可选年龄更新
};

/**
 * @class Reader
 * @brief 基础层读者实体类，维护读者的基础信息、借阅状态与信用数据。
 */
class Reader {
public:
    Reader();
    Reader(std::string readerId,
        std::string name,
        ContactInfo contact,
        int maxBorrow,
        int age = 0);

    // === 基本信息访问 ===
    [[nodiscard]] const std::string& getReaderId() const noexcept;
    void setReaderId(const std::string& readerId);

    [[nodiscard]] const std::string& getName() const noexcept;
    void setName(const std::string& name);

    [[nodiscard]] const ContactInfo& getContactInfo() const noexcept;
    void setContactInfo(const ContactInfo& contact);

    [[nodiscard]] int getAge() const noexcept;
    void setAge(int age);

    [[nodiscard]] int getMaxBorrow() const noexcept;
    void setMaxBorrow(int maxBorrow);

    // === 账户状态 ===
    [[nodiscard]] bool isActive() const noexcept;
    void activate();
    void deactivate();

    [[nodiscard]] bool isSuspended() const noexcept;
    [[nodiscard]] const Date& getSuspensionLiftDate() const noexcept;
    void suspendUntil(const Date& liftDate);
    void clearSuspension();

    [[nodiscard]] int getCreditScore() const noexcept;
    void setCreditScore(int score);
    void adjustCreditScore(int delta);

    // === 借阅统计 ===
    [[nodiscard]] int getCurrentBorrowCount() const noexcept;
    [[nodiscard]] int getTotalBorrowed() const noexcept;
    [[nodiscard]] const std::vector<std::string>& getCurrentBorrowedIsbns() const noexcept;
    [[nodiscard]] const std::vector<std::string>& getBorrowHistory() const noexcept;
    // 当前借阅操作
    bool addCurrentBorrow(const std::string& isbn);   // 借书
    bool removeCurrentBorrow(const std::string& isbn); // 还书
    // 历史记录
    void addBorrowHistoryEntry(const std::string& isbn); // 添加历史记录

    // === 预约与偏好 ===
    [[nodiscard]] const std::vector<std::string>& getReservations() const noexcept;
    void addReservation(const std::string& isbn);
    void removeReservation(const std::string& isbn);
    void clearReservations();

    // === 活跃时间 ===
    [[nodiscard]] const Date& getRegisterDate() const noexcept;
    [[nodiscard]] const Date& getLastActiveDate() const noexcept;
    void updateLastActiveDate(const Date& date);

    // === 数据更新 ===
    void applyUpdate(const ReaderUpdate& update);

    // === 序列化支持 ===
    [[nodiscard]] std::string serialize(char delimiter = '|') const;
    static Reader deserialize(const std::string& line, char delimiter = '|');

private:
    std::string m_readerId;      // 读者ID（唯一标识）
    std::string m_name;          // 读者姓名
    ContactInfo m_contactInfo;   // 联系方式
    int m_age{ 0 };               // 年龄（可选）
    int m_maxBorrow{ 5 };         // 最大可借阅数（默认5本）

    bool m_active{ true };               // 账户是否激活
    bool m_suspended{ false };           // 是否被暂停借阅
    Date m_suspensionLiftDate;         // 暂停解除日期
    int m_creditScore{ 600 };            // 信用评分（默认600分）

    Date m_registerDate;        // 注册日期
    Date m_lastActiveDate;      // 最后活跃日期

    int m_totalBorrowed{ 0 };                           // 历史借阅总数
    std::vector<std::string> m_currentBorrowedIsbns;  // 当前借阅书籍ISBN
    std::vector<std::string> m_borrowHistory;         // 借阅历史记录
    std::vector<std::string> m_reservations;          // 预约书籍列表
};

#endif // READER_H
