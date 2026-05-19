#ifndef CREDIT_SYSTEM_H
#define CREDIT_SYSTEM_H

#include "BorrowService.h"
#include "ReaderService.h"

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @enum CreditLevel
 * @brief 信用等级枚举。
 */
enum class CreditLevel {
    Suspended,
    Low,
    Medium,
    High,
    Premium
};

/**
 * @struct CreditReport
 * @brief 信用报告数据结构。
 */
struct CreditReport {
    std::string readerId;
    int score{ 0 };
    CreditLevel level{ CreditLevel::Low };
    std::vector<std::string> history;
};

/**
 * @struct Reward
 * @brief 信用奖励信息。
 */
struct Reward {
    std::string id;
    std::string name;
    std::string description;
    int requiredPoints{ 0 };
};

/**
 * @class CreditSystem
 * @brief 高级功能层信用体系，实现信用评分、积分奖励与违规处理。
 */
class CreditSystem {
public:
    CreditSystem(ReaderService& readerService, BorrowService& borrowService);

    // === 信用评分 ===
    int calculateCreditScore(const std::string& readerId);
    CreditLevel getCreditLevel(const std::string& readerId);
    CreditReport generateCreditReport(const std::string& readerId);

    // === 信用调整 ===
    void addCreditPoints(const std::string& readerId, int points, const std::string& reason);
    void deductCreditPoints(const std::string& readerId, int points, const std::string& reason);
    void resetCreditScore(const std::string& readerId, int defaultScore = 600);

    // === 权限管理 ===
    int getMaxBooksAllowed(const std::string& readerId);
    int getBorrowDaysAllowed(const std::string& readerId);
    double getFineDiscountRate(const std::string& readerId);

    // === 激励机制 ===
    std::vector<Reward> getAvailableRewards(const std::string& readerId);
    bool claimReward(const std::string& readerId, const std::string& rewardId);
    void grantRewardForGoodBehavior(const std::string& readerId);

    // === 违规处理 ===
    bool issueWarning(const std::string& readerId, const std::string& reason);
    bool suspendAccount(const std::string& readerId, int days);
    bool restoreAccount(const std::string& readerId);

private:
    ReaderService& m_readerService;
    BorrowService& m_borrowService;

    std::unordered_map<std::string, std::vector<std::string>> m_creditHistory;
    std::unordered_map<std::string, int> m_rewardPoints;

    std::vector<Reward> m_rewardCatalog;

    void initializeRewards();
    CreditLevel scoreToLevel(int score) const;
    void logHistory(const std::string& readerId, const std::string& entry);
    void adjustReaderScore(const std::string& readerId, int delta);
};

#endif // CREDIT_SYSTEM_H
