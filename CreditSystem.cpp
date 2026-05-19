#include "CreditSystem.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <algorithm>
#include <sstream>

// ==== 构造与初始化 =========================================================

// 构造函数
CreditSystem::CreditSystem(ReaderService& readerService, BorrowService& borrowService)
    : m_readerService(readerService),
    m_borrowService(borrowService) {
    initializeRewards();  // 初始化奖励目录
}

// 初始化奖励目录
void CreditSystem::initializeRewards() {
    m_rewardCatalog = {
        { "RWD-001", "罚金抵扣券", "抵扣 5 元逾期罚金", 200 },
        { "RWD-002", "优先预约权", "未来 30 天内可享一次预约优先权", 350 },
        { "RWD-003", "延长借阅期", "本月任意一次借阅可额外延长 7 天", 450 },
        { "RWD-004", "读书会邀请", "受邀参加图书馆 VIP 读书会", 600 }
    };
}

// ==== 信用评分 =============================================================

// 计算信用评分
int CreditSystem::calculateCreditScore(const std::string& readerId) {
    Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return 0;  // 读者不存在
    }

    int score = reader->getCreditScore();
    auto history = m_borrowService.getReaderBorrowHistory(readerId);

    int overdueCount = 0;
    int punctualReturns = 0;

    // 统计逾期记录和准时归还记录
    for (const auto& record : history) {
        bool returned = record.isReturned();
        Date comparisonDate = returned ? record.getReturnDate().value() : DateUtil::today();
        if (record.isOverdue(comparisonDate)) {
            ++overdueCount;  // 逾期计数
        }
        else if (returned && record.getReturnDate().value() <= record.getDueDate()) {
            ++punctualReturns;  // 准时归还计数
        }
    }

    // 根据记录调整分数
    score -= overdueCount * 15;  // 逾期扣分
    score += punctualReturns * 5;  // 准时归还加分

    // 鼓励及时归还（当前没有借阅且历史有借阅记录）
    if (reader->getCurrentBorrowCount() == 0 && reader->getTotalBorrowed() > 0) {
        score += 10;
    }

    score = std::clamp(score, 0, 1000);  // 将分数限制在0-1000范围内
    reader->setCreditScore(score);
    return score;
}

// 获取信用等级
CreditLevel CreditSystem::getCreditLevel(const std::string& readerId) {
    int score = calculateCreditScore(readerId);
    return scoreToLevel(score);
}

// 生成信用报告
CreditReport CreditSystem::generateCreditReport(const std::string& readerId) {
    CreditReport report{};
    report.readerId = readerId;

    Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return report;  // 读者不存在
    }

    report.score = calculateCreditScore(readerId);
    report.level = scoreToLevel(report.score);

    // 获取信用历史记录
    auto it = m_creditHistory.find(readerId);
    if (it != m_creditHistory.end()) {
        report.history = it->second;
    }

    return report;
}

// ==== 信用调整 =============================================================

// 增加信用积分
void CreditSystem::addCreditPoints(const std::string& readerId, int points, const std::string& reason) {
    if (points <= 0) {
        return;
    }
    adjustReaderScore(readerId, points);
    m_rewardPoints[readerId] += points;
    logHistory(readerId, "加分 +" + std::to_string(points) + " : " + reason);
}

// 扣除信用积分
void CreditSystem::deductCreditPoints(const std::string& readerId, int points, const std::string& reason) {
    if (points <= 0) {
        return;
    }
    adjustReaderScore(readerId, -points);
    m_rewardPoints[readerId] = std::max(0, m_rewardPoints[readerId] - points);
    logHistory(readerId, "扣分 -" + std::to_string(points) + " : " + reason);
}

// 重置信用分数
void CreditSystem::resetCreditScore(const std::string& readerId, int defaultScore) {
    Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return;
    }
    m_readerService.resetCreditScore(readerId, defaultScore);
    logHistory(readerId, "信用分重置为 " + std::to_string(defaultScore));
}

// ==== 权限管理 =============================================================

// 获取最大可借图书数量
int CreditSystem::getMaxBooksAllowed(const std::string& readerId) {
    CreditLevel level = getCreditLevel(readerId);
    Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return 0;
    }

    int base = reader->getMaxBorrow();
    switch (level) {
    case CreditLevel::Suspended: return 0;  // 暂停状态不能借书
    case CreditLevel::Low: return std::max(1, base - 2);  // 低信用等级减少借书数量
    case CreditLevel::Medium: return base;  // 中等信用等级使用基础数量
    case CreditLevel::High: return base + 2;  // 高信用等级增加借书数量
    case CreditLevel::Premium: return base + 4;  // 优质信用等级显著增加借书数量
    default: return base;
    }
}

// 获取借阅天数
int CreditSystem::getBorrowDaysAllowed(const std::string& readerId) {
    CreditLevel level = getCreditLevel(readerId);
    switch (level) {
    case CreditLevel::Suspended: return 0;
    case CreditLevel::Low: return 7;  // 低信用等级只能借7天
    case CreditLevel::Medium: return 14;  // 中等信用等级可借14天
    case CreditLevel::High: return 21;  // 高信用等级可借21天
    case CreditLevel::Premium: return 28;  // 优质信用等级可借28天
    default: return 14;
    }
}

// 获取罚金折扣率
double CreditSystem::getFineDiscountRate(const std::string& readerId) {
    CreditLevel level = getCreditLevel(readerId);
    switch (level) {
    case CreditLevel::Suspended: return 0.0;  // 无折扣
    case CreditLevel::Low: return 0.0;  // 无折扣
    case CreditLevel::Medium: return 0.05;  // 5%折扣
    case CreditLevel::High: return 0.10;  // 10%折扣
    case CreditLevel::Premium: return 0.20;  // 20%折扣
    default: return 0.0;
    }
}

// ==== 激励机制 =============================================================

// 获取可兑换的奖励
std::vector<Reward> CreditSystem::getAvailableRewards(const std::string& readerId) {
    std::vector<Reward> rewards;
    int points = m_rewardPoints[readerId];
    // 筛选积分足够的奖励
    for (const auto& reward : m_rewardCatalog) {
        if (points >= reward.requiredPoints) {
            rewards.push_back(reward);
        }
    }
    return rewards;
}

// 兑换奖励
bool CreditSystem::claimReward(const std::string& readerId, const std::string& rewardId) {
    auto it = std::find_if(m_rewardCatalog.begin(), m_rewardCatalog.end(),
        [&](const Reward& reward) { return reward.id == rewardId; });
    if (it == m_rewardCatalog.end()) {
        return false;  // 奖励不存在
    }

    int& points = m_rewardPoints[readerId];
    if (points < it->requiredPoints) {
        return false;  // 积分不足
    }

    points -= it->requiredPoints;  // 扣除积分
    logHistory(readerId, "兑换奖励：" + it->name + " (-" + std::to_string(it->requiredPoints) + " 积分)");
    return true;
}

// 授予良好行为奖励
void CreditSystem::grantRewardForGoodBehavior(const std::string& readerId) {
    addCreditPoints(readerId, 50, "优秀借阅行为奖励");
}

// ==== 违规处理 =============================================================

// 发出警告
bool CreditSystem::issueWarning(const std::string& readerId, const std::string& reason) {
    logHistory(readerId, "警告：" + reason);
    deductCreditPoints(readerId, 20, "违规警告：" + reason);
    return true;
}

// 暂停账户
bool CreditSystem::suspendAccount(const std::string& readerId, int days) {
    Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return false;
    }
    Date liftDate = DateUtil::addDays(DateUtil::today(), std::max(1, days));
    reader->suspendUntil(liftDate);
    reader->deactivate();
    logHistory(readerId, "账户暂停至 " + DateUtil::toString(liftDate));
    return true;
}

// 恢复账户
bool CreditSystem::restoreAccount(const std::string& readerId) {
    Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return false;
    }
    reader->clearSuspension();
    reader->activate();
    logHistory(readerId, "账户恢复正常状态");
    return true;
}

// ==== 内部工具 =============================================================

// 根据分数转换为信用等级
CreditLevel CreditSystem::scoreToLevel(int score) const {
    if (score <= 300) {
        return CreditLevel::Suspended;  // 暂停状态
    }
    else if (score <= 500) {
        return CreditLevel::Low;  // 低信用
    }
    else if (score <= 700) {
        return CreditLevel::Medium;  // 中等信用
    }
    else if (score <= 850) {
        return CreditLevel::High;  // 高信用
    }
    return CreditLevel::Premium;  // 优质信用
}

// 记录信用历史
void CreditSystem::logHistory(const std::string& readerId, const std::string& entry) {
    m_creditHistory[readerId].push_back(entry);
    // 保持历史记录不超过50条
    if (m_creditHistory[readerId].size() > 50) {
        m_creditHistory[readerId].erase(m_creditHistory[readerId].begin());
    }
}

// 调整读者分数
void CreditSystem::adjustReaderScore(const std::string& readerId, int delta) {
    Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return;
    }
    reader->adjustCreditScore(delta);
}
