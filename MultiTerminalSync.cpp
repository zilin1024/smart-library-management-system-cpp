#include "MultiTerminalSync.h"
#include "DateUtil.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

// ==== 构造函数 ==============================================================

// 初始化多终端同步管理器，绑定必要的服务引用，并设置默认同步间隔
MultiTerminalSync::MultiTerminalSync(LibraryManager& libraryManager,
    ReaderService& readerService,
    BorrowService& borrowService,
    FileManager& fileManager)
    : m_libraryManager(libraryManager),
    m_readerService(readerService),
    m_borrowService(borrowService),
    m_fileManager(fileManager),
    m_autoSyncInterval(std::chrono::minutes(60)), // 默认自动同步间隔60分钟
    m_autoSyncEnabled(false) {} // 默认关闭自动同步

// ==== 终端管理 ==============================================================

// 注册新设备：检查ID和名称是否为空，以及ID是否已存在
bool MultiTerminalSync::registerDevice(const DeviceInfo& device) {
    if (device.deviceId.empty() || device.deviceName.empty()) {
        return false;
    }
    if (m_devices.contains(device.deviceId)) {
        return false;
    }

    DeviceInfo info = device;
    // 如果上次同步时间无效，默认为今天
    if (!DateUtil::isValid(info.lastSyncDate)) {
        info.lastSyncDate = DateUtil::today();
    }
    info.status = SyncStatus::Idle; // 初始状态为空闲

    m_devices.emplace(info.deviceId, info);
    m_lastResults[info.deviceId] = {}; // 初始化该设备的结果记录
    return true;
}

// 注销设备：移除设备信息及相关联的计划、结果和冲突记录
bool MultiTerminalSync::unregisterDevice(const std::string& deviceId) {
    bool erased = m_devices.erase(deviceId) > 0;
    if (erased) {
        m_schedule.erase(deviceId);
        m_lastResults.erase(deviceId);
        m_pendingConflicts.erase(deviceId);
    }
    return erased;
}

// 更新设备信息：仅更新非空字段，保持原状态
bool MultiTerminalSync::updateDeviceInfo(const std::string& deviceId, const DeviceInfo& update) {
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end()) {
        return false;
    }

    DeviceInfo& device = it->second;
    // 仅当 update 中的字段非空时才更新
    device.deviceName = update.deviceName.empty() ? device.deviceName : update.deviceName;
    device.location = update.location.empty() ? device.location : update.location;
    if (DateUtil::isValid(update.lastSyncDate)) {
        device.lastSyncDate = update.lastSyncDate;
    }
    device.status = update.status;
    return true;
}

// 获取设备详情
std::optional<DeviceInfo> MultiTerminalSync::getDeviceInfo(const std::string& deviceId) const {
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end()) {
        return std::nullopt;
    }
    return it->second;
}

// 列出所有已注册设备
std::vector<DeviceInfo> MultiTerminalSync::listDevices() const {
    std::vector<DeviceInfo> result;
    result.reserve(m_devices.size());
    for (const auto& [id, info] : m_devices) {
        (void)id;
        result.push_back(info);
    }
    return result;
}

// ==== 同步控制 ==============================================================

// 立即执行同步操作
SyncResult MultiTerminalSync::syncNow(const std::string& deviceId) {
    SyncResult result;
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end()) {
        result.status = SyncStatus::Failed;
        result.message = "设备未注册。";
        logSyncResult(deviceId, result);
        return result;
    }

    DeviceInfo& device = it->second;
    device.status = SyncStatus::InProgress; // 标记为正在同步

    try {
        // 1. 创建同步快照（模拟）
        std::string snapshot = makeSyncSnapshot(deviceId);
        (void)snapshot;

        // 2. 将数据写入本地存储（持久化当前状态）
        if (!m_fileManager.saveAllData()) {
            throw std::runtime_error("保存本地数据失败。");
        }

        // 3. 从终端拉取变化（模拟）
        std::string deviceChanges;
        if (!pullDeviceChanges(deviceId, deviceChanges)) {
            // 处理拉取失败逻辑（当前略过）
        }

        // 4. 更新终端状态为完成
        device.lastSyncDate = DateUtil::today();
        device.status = SyncStatus::Completed;

        // 5. 填充成功结果
        result.status = SyncStatus::Completed;
        result.message = "同步完成。";
        result.booksSynced = m_libraryManager.getTotalBookCount();
        result.readersSynced = m_libraryManager.getTotalReaderCount();
        result.recordsSynced = m_libraryManager.getTotalBorrowRecordCount();
    }
    catch (const std::exception& e) {
        // 异常处理：更新状态为失败
        device.status = SyncStatus::Failed;
        result.status = SyncStatus::Failed;
        result.message = std::string("同步失败：") + e.what();
    }

    m_schedule.erase(deviceId); // 同步完成后移除计划任务
    logSyncResult(deviceId, result);
    return result;
}

// 安排未来的同步任务
bool MultiTerminalSync::scheduleSync(const std::string& deviceId, Date targetDate) {
    if (!m_devices.contains(deviceId) || !DateUtil::isValid(targetDate)) {
        return false;
    }
    m_schedule[deviceId] = targetDate;
    return true;
}

// 取消计划中的同步
bool MultiTerminalSync::cancelScheduledSync(const std::string& deviceId) {
    return m_schedule.erase(deviceId) > 0;
}

// 获取计划同步的日期
std::optional<Date> MultiTerminalSync::getScheduledSync(const std::string& deviceId) const {
    auto it = m_schedule.find(deviceId);
    if (it == m_schedule.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ==== 冲突处理 ==============================================================

// 检测同步冲突
std::vector<ConflictEntry> MultiTerminalSync::detectConflicts(const std::string& deviceId) {
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end()) {
        return {};
    }
    auto& conflicts = m_pendingConflicts[deviceId];
    conflicts.clear(); 
    return conflicts;
}

// 解决冲突：接受解决方案列表
bool MultiTerminalSync::resolveConflicts(const std::string& deviceId,
    const std::vector<ConflictEntry>& resolutions) {
    if (!m_devices.contains(deviceId)) {
        return false;
    }

    auto& pending = m_pendingConflicts[deviceId];
    pending.clear();
    // 模拟应用解决方案
    for (const auto& entry : resolutions) {
        pending.push_back(entry);
    }
    pending.clear(); 
    return true;
}

// ==== 状态查询 ==============================================================

// 获取设备当前同步状态
SyncStatus MultiTerminalSync::getDeviceStatus(const std::string& deviceId) const {
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end()) {
        return SyncStatus::Failed;
    }
    return it->second.status;
}

// 获取上次同步结果
SyncResult MultiTerminalSync::getLastSyncResult(const std::string& deviceId) const {
    auto it = m_lastResults.find(deviceId);
    if (it == m_lastResults.end()) {
        return {};
    }
    return it->second;
}

// ==== 配置 =================================================================

// 设置自动同步间隔
void MultiTerminalSync::setAutoSyncInterval(std::chrono::minutes interval) {
    if (interval.count() <= 0) {
        interval = std::chrono::minutes(60);
    }
    m_autoSyncInterval = interval;
}

std::chrono::minutes MultiTerminalSync::getAutoSyncInterval() const {
    return m_autoSyncInterval;
}

// 启用/禁用自动同步
void MultiTerminalSync::enableAutoSync(bool enabled) {
    m_autoSyncEnabled = enabled;
}

bool MultiTerminalSync::isAutoSyncEnabled() const {
    return m_autoSyncEnabled;
}

// ==== 内部工具 ==============================================================

// 创建同步快照：保存当前所有数据
std::string MultiTerminalSync::makeSyncSnapshot(const std::string& deviceId) {
    (void)deviceId;
    if (!m_fileManager.saveAllData()) {
        throw std::runtime_error("创建快照失败。");
    }
    return "snapshot.dat"; // 返回虚拟快照文件名
}

// 应用快照到设备（桩函数）
bool MultiTerminalSync::applySnapshotToDevice(const std::string& deviceId, const std::string& snapshotFile) {
    (void)deviceId;
    (void)snapshotFile;
    return true;
}

// 拉取设备变更（桩函数）
bool MultiTerminalSync::pullDeviceChanges(const std::string& deviceId, std::string& changesFile) {
    (void)deviceId;
    changesFile.clear();
    return true;
}

// 记录同步结果到内存
void MultiTerminalSync::logSyncResult(const std::string& deviceId, const SyncResult& result) {
    m_lastResults[deviceId] = result;
}

