#include "SecurityManager.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

// ==== 工具函数 ==============================================================

namespace {
    // 将审计严重程度转换为整数权重，用于排序和比较
    int severityRank(AuditSeverity severity) {
        switch (severity) {
        case AuditSeverity::Info: return 0;
        case AuditSeverity::Warning: return 1;
        case AuditSeverity::Critical: return 2;
        default: return 0;
        }
    }
} // namespace

// ==== 构造函数 ==============================================================

// 构造函数：初始化依赖服务并设置默认安全策略
SecurityManager::SecurityManager(ReaderService& readerService,
    BorrowService& borrowService,
    FileManager& fileManager)
    : m_readerService(readerService),
    m_borrowService(borrowService),
    m_fileManager(fileManager) {
    // 默认策略：允许所有角色访问，生效期为当前日起一年
    Date today = DateUtil::today();
    m_defaultPolicy.policyId = "DEFAULT";
    m_defaultPolicy.allowedRoles = { "READER", "ADMIN" };
    m_defaultPolicy.restrictedResources = {};
    m_defaultPolicy.requireMfa = false;
    m_defaultPolicy.effectiveFrom = today;
    m_defaultPolicy.effectiveTo = DateUtil::addDays(today, 365);
}

// ==== 访问控制 ==============================================================

// 为读者注册/分配角色
bool SecurityManager::registerRole(const std::string& readerId, const std::string& role) {
    if (!m_readerService.checkReaderIdExists(readerId) || role.empty()) {
        return false;
    }
    m_readerRoles[readerId].insert(StringUtil::toUpper(role));
    logAudit(readerId, "AssignRole", AuditSeverity::Info, "角色授予：" + role);
    return true;
}

// 撤销读者的角色
bool SecurityManager::revokeRole(const std::string& readerId, const std::string& role) {
    auto it = m_readerRoles.find(readerId);
    if (it == m_readerRoles.end()) {
        return false;
    }
    auto& roles = it->second;
    auto erased = roles.erase(StringUtil::toUpper(role));
    if (erased > 0) {
        logAudit(readerId, "RevokeRole", AuditSeverity::Info, "角色撤销：" + role);
        return true;
    }
    return false;
}

// 核心权限检查：验证读者状态、策略有效期、角色权限及 MFA
bool SecurityManager::checkAccess(const std::string& readerId, const std::string& resource) const {
    // 1. 检查读者账户状态
    const Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr || !reader->isActive() || reader->isSuspended()) {
        return false;
    }

    // 2. 获取资源对应的策略并检查有效期
    AccessPolicy policy = resolvePolicyForResource(resource);
    Date today = DateUtil::today();
    if (policy.effectiveFrom > today || policy.effectiveTo < today) {
        return false;
    }

    // 3. 获取用户角色（默认为 READER）
    auto it = m_readerRoles.find(readerId);
    std::unordered_set<std::string> roles;
    if (it != m_readerRoles.end()) {
        roles.insert(it->second.begin(), it->second.end());
    }

    if (roles.empty()) {
        roles.insert("READER"); // 默认角色
    }

    // 4. 检查角色是否被策略允许
    if (!policy.allowedRoles.empty()) {
        bool allowed = false;
        for (const auto& role : roles) {
            if (policy.allowedRoles.count(role) > 0) {
                allowed = true;
                break;
            }
        }
        if (!allowed) {
            return false;
        }
    }

    // 5. 检查是否需要多因素认证 (MFA)
    if (policy.requireMfa) {
        bool hasMfaSession = false;
        for (const auto& [sessionId, session] : m_sessions) {
            (void)sessionId;
            if (session.readerId == readerId && session.active && session.mfaPassed) {
                hasMfaSession = true;
                break;
            }
        }
        if (!hasMfaSession) {
            return false;
        }
    }

    return true;
}

void SecurityManager::setDefaultPolicy(const AccessPolicy& policy) {
    m_defaultPolicy = policy;
}

// 添加自定义访问策略
bool SecurityManager::addPolicy(const AccessPolicy& policy) {
    if (policy.policyId.empty()) {
        return false;
    }
    m_policies[policy.policyId] = policy;
    return true;
}

bool SecurityManager::removePolicy(const std::string& policyId) {
    return m_policies.erase(policyId) > 0;
}

// ==== 会话管理 ==============================================================

// 创建新会话
std::string SecurityManager::createSession(const std::string& readerId) {
    if (!m_readerService.checkReaderIdExists(readerId)) {
        return {};
    }
    SessionInfo session;
    session.sessionId = generateSessionId();
    session.readerId = readerId;
    session.loginDate = DateUtil::today();
    session.mfaPassed = false;
    session.active = true;

    m_sessions[session.sessionId] = session;
    logAudit(readerId, "CreateSession", AuditSeverity::Info, "新会话创建：" + session.sessionId);
    return session.sessionId;
}

// 关闭会话
bool SecurityManager::closeSession(const std::string& sessionId) {
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return false;
    }
    it->second.active = false;
    logAudit(it->second.readerId, "CloseSession", AuditSeverity::Info, "会话关闭：" + sessionId);
    return true;
}

// 更新会话的 MFA 验证状态
bool SecurityManager::markSessionMfa(const std::string& sessionId, bool passed) {
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return false;
    }
    it->second.mfaPassed = passed;
    logAudit(it->second.readerId, "MfaUpdate", AuditSeverity::Info,
        std::string("MFA 验证状态：") + (passed ? "通过" : "未通过"));
    return true;
}

std::optional<SessionInfo> SecurityManager::getSession(const std::string& sessionId) const {
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ==== 审计日志 ==============================================================

// 记录审计日志（并限制最大日志条数，防止无限增长）
void SecurityManager::logAudit(const std::string& actorId,
    const std::string& action,
    AuditSeverity severity,
    const std::string& details) {
    AuditEntry entry;
    entry.entryId = generateAuditId();
    entry.timestamp = DateUtil::today();
    entry.actorId = actorId;
    entry.action = action;
    entry.severity = severity;
    entry.details = details;

    m_auditTrail.push_back(entry);

    // 限制日志长度
    if (m_auditTrail.size() > 1000) {
        m_auditTrail.erase(m_auditTrail.begin(), m_auditTrail.begin() + 100);
    }
}

// 查询审计日志
std::vector<AuditEntry> SecurityManager::queryAuditLogs(Date start, Date end, AuditSeverity minSeverity) const {
    if (end < start) {
        std::swap(start, end);
    }
    std::vector<AuditEntry> results;
    for (const auto& entry : m_auditTrail) {
        if (entry.timestamp >= start && entry.timestamp <= end &&
            severityRank(entry.severity) >= severityRank(minSeverity)) {
            results.push_back(entry);
        }
    }
    return results;
}

// ==== 风险与告警 ==============================================================

// 触发安全告警
bool SecurityManager::triggerAlert(const std::string& readerId,
    const std::string& description,
    AuditSeverity severity) {
    if (!m_readerService.checkReaderIdExists(readerId) || description.empty()) {
        return false;
    }
    SecurityAlert alert;
    alert.alertId = generateAlertId();
    alert.triggeredAt = DateUtil::today();
    alert.readerId = readerId;
    alert.description = description;
    alert.severity = severity;
    alert.acknowledged = false;

    m_alerts[alert.alertId] = alert;
    logAudit(readerId, "TriggerAlert", severity, description);
    return true;
}

// 列出告警（可选是否包含已确认的）
std::vector<SecurityAlert> SecurityManager::listAlerts(bool includeAcknowledged) const {
    std::vector<SecurityAlert> result;
    for (const auto& [id, alert] : m_alerts) {
        (void)id;
        if (!includeAcknowledged && alert.acknowledged) {
            continue;
        }
        result.push_back(alert);
    }
    // 按严重程度降序排序
    std::sort(result.begin(), result.end(), [](const SecurityAlert& lhs, const SecurityAlert& rhs) {
        if (lhs.severity == rhs.severity) {
            return lhs.alertId < rhs.alertId;
        }
        return severityRank(lhs.severity) > severityRank(rhs.severity);
        });
    return result;
}

// 确认/处理告警
bool SecurityManager::acknowledgeAlert(const std::string& alertId) {
    auto it = m_alerts.find(alertId);
    if (it == m_alerts.end()) {
        return false;
    }
    it->second.acknowledged = true;
    logAudit(it->second.readerId, "AcknowledgeAlert", AuditSeverity::Info, "告警确认：" + alertId);
    return true;
}

// ==== 风险检测（占位实现） ====================================================

// 执行风险评估：检测逾期和频繁违规
void SecurityManager::performRiskAssessment() {
    auto overdueRecords = m_borrowService.getOverdueRecords();
    std::unordered_map<std::string, int> overdueCounter;
    Date today = DateUtil::today();

    for (const auto& record : overdueRecords) {
        overdueCounter[record.getReaderId()]++;
        if (record.isOverdue(today)) {
            triggerAlert(record.getReaderId(),
                "借阅记录逾期：" + record.getISBN(),
                AuditSeverity::Warning);
        }
    }

    // 如果读者频繁逾期（>=3次），触发严重告警
    for (const auto& [readerId, count] : overdueCounter) {
        if (count >= 3) {
            triggerAlert(readerId,
                "频繁逾期次数：" + std::to_string(count),
                AuditSeverity::Critical);
        }
    }
}

// 获取高风险读者列表（未确认的严重告警）
std::vector<std::string> SecurityManager::getHighRiskReaders() const {
    std::vector<std::string> risky;
    for (const auto& [id, alert] : m_alerts) {
        (void)id;
        if (alert.severity == AuditSeverity::Critical && !alert.acknowledged) {
            risky.push_back(alert.readerId);
        }
    }
    std::sort(risky.begin(), risky.end());
    risky.erase(std::unique(risky.begin(), risky.end()), risky.end());
    return risky;
}

// ==== 私有工具 ==============================================================

// 根据资源名称解析对应的访问策略
AccessPolicy SecurityManager::resolvePolicyForResource(const std::string& resource) const {
    for (const auto& [policyId, policy] : m_policies) {
        (void)policyId;
        if (policy.restrictedResources.empty()) {
            continue;
        }
        if (policy.restrictedResources.count(resource) > 0) {
            return policy;
        }
    }
    return m_defaultPolicy;
}

// ID生成器
std::string SecurityManager::generateAuditId() {
    ++m_auditSequence;
    return "AUD-" + std::to_string(m_auditSequence);
}

std::string SecurityManager::generateAlertId() {
    ++m_alertSequence;
    return "ALT-" + std::to_string(m_alertSequence);
}

std::string SecurityManager::generateSessionId() {
    ++m_sessionSequence;
    return "SES-" + std::to_string(m_sessionSequence);
}

