#include "ReaderService.h"
#include <algorithm>
#include <stdexcept>

// ==== 构造函数 =============================================================

ReaderService::ReaderService(LibraryManager& manager)
    : m_library(manager) {}

// ==== 读者注册 =============================================================

// 注册新读者：验证ID唯一性后存入库
bool ReaderService::registerReader(const Reader& reader) {
    if (reader.getReaderId().empty()) {
        return false;
    }
    if (checkReaderIdExists(reader.getReaderId())) {
        return false;
    }
    return m_library.addReader(reader);
}

// 检查读者ID是否存在
bool ReaderService::checkReaderIdExists(const std::string& readerId) const {
    return m_library.getReader(readerId) != nullptr;
}

// ==== 读者信息修改 =========================================================

// 更新读者基本信息（支持部分更新）
bool ReaderService::updateReaderInfo(const std::string& readerId, const ReaderUpdate& update) {
    if (!checkReaderIdExists(readerId)) {
        return false;
    }
    return m_library.updateReader(readerId, [&](Reader& reader) {
        reader.applyUpdate(update); // 调用 Reader 内置的批量更新逻辑
        });
}

// 单独更新联系方式
bool ReaderService::updateContactInfo(const std::string& readerId, const ContactInfo& contact) {
    if (!checkReaderIdExists(readerId)) {
        return false;
    }
    return m_library.updateReader(readerId, [&](Reader& reader) {
        reader.setContactInfo(contact);
        });
}

// 更新最大借阅数量
bool ReaderService::updateMaxBooks(const std::string& readerId, int maxBooks) {
    if (maxBooks <= 0 || !checkReaderIdExists(readerId)) {
        return false;
    }
    return m_library.updateReader(readerId, [&](Reader& reader) {
        reader.setMaxBorrow(maxBooks);
        });
}

// ==== 读者状态管理 =========================================================

// 停用读者账户（软删除）
bool ReaderService::deactivateReader(const std::string& readerId) {
    if (!checkReaderIdExists(readerId)) {
        return false;
    }
    return m_library.updateReader(readerId, [](Reader& reader) {
        reader.deactivate();
        });
}

// 激活读者账户，并清除停借状态
bool ReaderService::activateReader(const std::string& readerId) {
    if (!checkReaderIdExists(readerId)) {
        return false;
    }
    return m_library.updateReader(readerId, [](Reader& reader) {
        reader.activate();
        reader.clearSuspension();
        });
}

// 永久删除读者（物理删除）
bool ReaderService::permanentlyDeleteReader(const std::string& readerId) {
    if (!checkReaderIdExists(readerId)) {
        return false;
    }
    return m_library.removeReader(readerId);
}

// ==== 查询功能 =============================================================

// 获取读者对象指针（可变）
Reader* ReaderService::getReaderById(const std::string& readerId) {
    return m_library.getReader(readerId);
}

// 获取读者对象指针（只读）
const Reader* ReaderService::getReaderById(const std::string& readerId) const {
    return m_library.getReader(readerId);
}

// 按姓名模糊搜索
std::vector<Reader> ReaderService::searchReadersByName(const std::string& name) const {
    std::vector<Reader> result;
    auto readers = m_library.getAllReaders();
    for (const auto& reader : readers) {
        if (StringUtil::containsIgnoreCase(reader.getName(), name)) {
            result.push_back(reader);
        }
    }
    return result;
}

// 获取累计借阅量达到阈值的读者
std::vector<Reader> ReaderService::getReadersByBorrowCount(int minCount) const {
    std::vector<Reader> result;
    if (minCount < 0) {
        minCount = 0;
    }

    auto readers = m_library.getAllReaders();
    for (const auto& reader : readers) {
        if (reader.getTotalBorrowed() >= minCount) {
            result.push_back(reader);
        }
    }
    return result;
}

// ==== 统计分析 =============================================================

// 统计活跃读者数量
int ReaderService::getActiveReaderCount() const {
    auto readers = m_library.getAllReaders();
    return static_cast<int>(std::count_if(readers.begin(), readers.end(), [](const Reader& reader) {
        return reader.isActive();
        }));
}

// 统计读者年龄分布
std::map<int, int> ReaderService::getReaderAgeDistribution() const {
    std::map<int, int> distribution;
    auto readers = m_library.getAllReaders();
    for (const auto& reader : readers) {
        if (reader.getAge() >= 0) {
            distribution[reader.getAge()]++;
        }
    }
    return distribution;
}

// 获取借阅量排行榜 Top N
std::vector<Reader> ReaderService::getTopBorrowers(int limit) const {
    auto readers = m_library.getAllReaders();
    std::sort(readers.begin(), readers.end(), [](const Reader& lhs, const Reader& rhs) {
        // 优先按借阅量降序，相同则按ID升序
        if (lhs.getTotalBorrowed() == rhs.getTotalBorrowed()) {
            return lhs.getReaderId() < rhs.getReaderId();
        }
        return lhs.getTotalBorrowed() > rhs.getTotalBorrowed();
        });

    if (limit <= 0 || static_cast<size_t>(limit) >= readers.size()) {
        return readers;
    }
    return std::vector<Reader>(readers.begin(), readers.begin() + limit);
}

// ==== 信用操作 =============================================================

// 调整读者信用分
bool ReaderService::adjustCreditScore(const std::string& readerId, int delta) {
    if (!checkReaderIdExists(readerId)) {
        return false;
    }
    return m_library.updateReader(readerId, [&](Reader& reader) {
        reader.adjustCreditScore(delta);
        });
}

// 重置读者信用分
bool ReaderService::resetCreditScore(const std::string& readerId, int newScore) {
    if (newScore < 0 || !checkReaderIdExists(readerId)) {
        return false;
    }
    return m_library.updateReader(readerId, [&](Reader& reader) {
        reader.setCreditScore(newScore);
        });
}

// ==== 私有工具 =============================================================

// 确保读者存在，否则抛出异常
void ReaderService::ensureReaderExists(const std::string& readerId) const {
    if (!checkReaderIdExists(readerId)) {
        throw std::runtime_error("Reader not found: " + readerId);
    }
}
