#ifndef SECURITY_MANAGER_H
#define SECURITY_MANAGER_H

#include "BorrowService.h"
#include "FileManager.h"
#include "ReaderService.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @enum AuditSeverity
 * @brief 审计日志级别。
 */
enum class AuditSeverity {
    Info,
    Warning,
    Critical
};

/**
 * @struct AuditEntry
 * @brief 安全审计日志条目。
 */
struct AuditEntry {
    std::string entryId;
    Date timestamp;
    std::string actorId;
    std::string action;
    AuditSeverity severity{ AuditSeverity::Info };
    std::string details;
};

/**
 * @struct AccessPolicy
 * @brief 访问控制策略。
 */
struct AccessPolicy {
    std::string policyId;
    std::set<std::string> allowedRoles;
    std::set<std::string> restrictedResources;
    bool requireMfa{ false };
    Date effectiveFrom;
    Date effectiveTo;
};

/**
 * @struct SecurityAlert
 * @brief 安全告警信息。
 */
struct SecurityAlert {
    std::string alertId;
    Date triggeredAt;
    std::string readerId;
    std::string description;
    AuditSeverity severity{ AuditSeverity::Warning };
    bool acknowledged{ false };
};

/**
 * @struct SessionInfo
 * @brief 会话信息，用于追踪登录状态。
 */
struct SessionInfo {
    std::string sessionId;
    std::string readerId;
    Date loginDate;
    bool mfaPassed{ false };
    bool active{ true };
};

/**
 * @class SecurityManager
 * @brief 安全管理模块，负责访问控制、风险检测、审计与告警。
 */
class SecurityManager {
public:
    SecurityManager(ReaderService& readerService,
        BorrowService& borrowService,
        FileManager& fileManager);

    // === 访问控制 ===
    bool registerRole(const std::string& readerId, const std::string& role);
    bool revokeRole(const std::string& readerId, const std::string& role);
    bool checkAccess(const std::string& readerId, const std::string& resource) const;
    void setDefaultPolicy(const AccessPolicy& policy);
    bool addPolicy(const AccessPolicy& policy);
    bool removePolicy(const std::string& policyId);

    // === 会话管理 ===
    std::string createSession(const std::string& readerId);
    bool closeSession(const std::string& sessionId);
    bool markSessionMfa(const std::string& sessionId, bool passed);
    std::optional<SessionInfo> getSession(const std::string& sessionId) const;

    // === 审计日志 ===
    void logAudit(const std::string& actorId,
        const std::string& action,
        AuditSeverity severity,
        const std::string& details);
    std::vector<AuditEntry> queryAuditLogs(Date start, Date end, AuditSeverity minSeverity) const;

    // === 风险与告警 ===
    bool triggerAlert(const std::string& readerId,
        const std::string& description,
        AuditSeverity severity);
    std::vector<SecurityAlert> listAlerts(bool includeAcknowledged = false) const;
    bool acknowledgeAlert(const std::string& alertId);

    // === 风险检测（占位实现） ===
    void performRiskAssessment();
    std::vector<std::string> getHighRiskReaders() const;

private:
    ReaderService& m_readerService;
    BorrowService& m_borrowService;
    FileManager& m_fileManager;

    AccessPolicy m_defaultPolicy;
    std::unordered_map<std::string, AccessPolicy> m_policies;
    std::unordered_map<std::string, std::set<std::string>> m_readerRoles;

    std::unordered_map<std::string, SessionInfo> m_sessions;
    std::vector<AuditEntry> m_auditTrail;
    std::unordered_map<std::string, SecurityAlert> m_alerts;

    long long m_auditSequence{ 0 };
    long long m_alertSequence{ 0 };
    long long m_sessionSequence{ 0 };

    AccessPolicy resolvePolicyForResource(const std::string& resource) const;
    std::string generateAuditId();
    std::string generateAlertId();
    std::string generateSessionId();
};

#endif // SECURITY_MANAGER_H
