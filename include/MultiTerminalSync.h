#ifndef MULTI_TERMINAL_SYNC_H
#define MULTI_TERMINAL_SYNC_H

#include "BorrowService.h"
#include "FileManager.h"
#include "LibraryManager.h"
#include "ReaderService.h"

#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @enum SyncStatus
 * @brief 同步状态枚举。
 */
enum class SyncStatus {
    Idle,
    InProgress,
    Completed,
    Failed,
    Conflict
};

/**
 * @struct SyncResult
 * @brief 同步结果信息。
 */
struct SyncResult {
    SyncStatus status{ SyncStatus::Idle };
    std::string message;
    int booksSynced{ 0 };
    int readersSynced{ 0 };
    int recordsSynced{ 0 };
};

/**
 * @struct DeviceInfo
 * @brief 同步终端信息。
 */
struct DeviceInfo {
    std::string deviceId;
    std::string deviceName;
    std::string location;
    Date lastSyncDate;
    SyncStatus status{ SyncStatus::Idle };
};

/**
 * @struct ConflictEntry
 * @brief 数据冲突条目。
 */
struct ConflictEntry {
    std::string entityId;
    std::string entityType;
    std::string localVersion;
    std::string remoteVersion;
    std::string resolution; // keep-local / keep-remote / merged
};

/**
 * @class MultiTerminalSync
 * @brief 多终端同步管理器，负责终端注册、数据同步与冲突处理。
 */
class MultiTerminalSync {
public:
    MultiTerminalSync(LibraryManager& libraryManager,
        ReaderService& readerService,
        BorrowService& borrowService,
        FileManager& fileManager);

    // === 终端管理 ===
    bool registerDevice(const DeviceInfo& device);
    bool unregisterDevice(const std::string& deviceId);
    bool updateDeviceInfo(const std::string& deviceId, const DeviceInfo& update);
    std::optional<DeviceInfo> getDeviceInfo(const std::string& deviceId) const;
    std::vector<DeviceInfo> listDevices() const;

    // === 同步控制 ===
    SyncResult syncNow(const std::string& deviceId);
    bool scheduleSync(const std::string& deviceId, Date targetDate);
    bool cancelScheduledSync(const std::string& deviceId);
    std::optional<Date> getScheduledSync(const std::string& deviceId) const;

    // === 冲突处理 ===
    std::vector<ConflictEntry> detectConflicts(const std::string& deviceId);
    bool resolveConflicts(const std::string& deviceId, const std::vector<ConflictEntry>& resolutions);

    // === 状态查询 ===
    SyncStatus getDeviceStatus(const std::string& deviceId) const;
    SyncResult getLastSyncResult(const std::string& deviceId) const;

    // === 配置 ===
    void setAutoSyncInterval(std::chrono::minutes interval);
    std::chrono::minutes getAutoSyncInterval() const;
    void enableAutoSync(bool enabled);
    bool isAutoSyncEnabled() const;

private:
    LibraryManager& m_libraryManager;
    ReaderService& m_readerService;
    BorrowService& m_borrowService;
    FileManager& m_fileManager;

    std::unordered_map<std::string, DeviceInfo> m_devices;
    std::unordered_map<std::string, Date> m_schedule;
    std::unordered_map<std::string, SyncResult> m_lastResults;
    std::unordered_map<std::string, std::vector<ConflictEntry>> m_pendingConflicts;

    std::chrono::minutes m_autoSyncInterval{ 60 };
    bool m_autoSyncEnabled{ false };

    std::string makeSyncSnapshot(const std::string& deviceId);
    bool applySnapshotToDevice(const std::string& deviceId, const std::string& snapshotFile);
    bool pullDeviceChanges(const std::string& deviceId, std::string& changesFile);
    void logSyncResult(const std::string& deviceId, const SyncResult& result);
};

#endif // MULTI_TERMINAL_SYNC_H
