#ifndef READER_SERVICE_H
#define READER_SERVICE_H

#include "LibraryManager.h"
#include "Reader.h"
#include "StringUtil.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

/**
 * @class ReaderService
 * @brief 读者业务服务层，封装注册、信息维护、注销与统计分析等逻辑。
 */
class ReaderService {
public:
    explicit ReaderService(LibraryManager& manager);

    // ===== 1. 读者注册 =====
    bool registerReader(const Reader& reader);
    bool checkReaderIdExists(const std::string& readerId) const;

    // ===== 2. 读者信息修改 =====
    bool updateReaderInfo(const std::string& readerId, const ReaderUpdate& update);
    bool updateContactInfo(const std::string& readerId, const ContactInfo& contact);
    bool updateMaxBooks(const std::string& readerId, int maxBooks);

    // ===== 3. 读者状态管理 =====
    bool deactivateReader(const std::string& readerId);
    bool activateReader(const std::string& readerId);
    bool permanentlyDeleteReader(const std::string& readerId);

    // ===== 4. 查询功能 =====
    Reader* getReaderById(const std::string& readerId);
    const Reader* getReaderById(const std::string& readerId) const;
    std::vector<Reader> searchReadersByName(const std::string& name) const;
    std::vector<Reader> getReadersByBorrowCount(int minCount) const;

    // ===== 5. 统计分析 =====
    int getActiveReaderCount() const;
    std::map<int, int> getReaderAgeDistribution() const;
    std::vector<Reader> getTopBorrowers(int limit = 10) const;

    // ===== 6. 信用操作 =====
    bool adjustCreditScore(const std::string& readerId, int delta);
    bool resetCreditScore(const std::string& readerId, int newScore = 600);

private:
    LibraryManager& m_library;

    void ensureReaderExists(const std::string& readerId) const;
};

#endif // READER_SERVICE_H
