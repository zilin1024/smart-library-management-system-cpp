#include "BorrowService.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <algorithm>
#include <cmath>
#include <sstream>

// ==== 构造 =================================================================

// 构造函数
BorrowService::BorrowService(LibraryManager& manager, int defaultBorrowDays, int maxRenewals)
    : m_library(manager),
    m_defaultBorrowDays(defaultBorrowDays),
    m_maxRenewals(maxRenewals) {}

// ==== 借书相关 =============================================================

// 借阅图书
BorrowResult BorrowService::borrowBook(const std::string& readerId, const std::string& isbn) {
    // 检查读者是否存在
    Reader* readerPtr = m_library.getReader(readerId);
    if (readerPtr == nullptr) {
        return makeBorrowResult(BorrowStatus::ReaderNotFound, "读者不存在。");
    }

    // 更新读者信息，释放过期的暂停
    m_library.updateReader(readerId, [&](Reader& reader) {
        releaseSuspensionIfExpired(reader);
        });
    readerPtr = m_library.getReader(readerId);
    if (readerPtr == nullptr) {
        return makeBorrowResult(BorrowStatus::ReaderNotFound, "读者不存在。");
    }

    // 检查读者状态
    if (!readerPtr->isActive()) {
        return makeBorrowResult(BorrowStatus::Failed, "读者未处于激活状态。");
    }
    if (readerPtr->isSuspended()) {
        return makeBorrowResult(BorrowStatus::SuspendedAccount, "读者账户被暂停使用。");
    }
    if (readerPtr->getCurrentBorrowCount() >= readerPtr->getMaxBorrow()) {
        return makeBorrowResult(BorrowStatus::LimitExceeded, "已达到最大借书数量。");
    }

    // 检查图书状态
    Book* bookPtr = m_library.getBook(isbn);
    if (bookPtr == nullptr) {
        return makeBorrowResult(BorrowStatus::BookNotFound, "图书不存在。");
    }
    if (bookPtr->getStock() <= 0) {
        return makeBorrowResult(BorrowStatus::BookOutOfStock, "图书库存不足。");
    }

    // 检查是否已借阅此书
    auto activeRecord = findActiveBorrowRecord(readerId, isbn);
    if (activeRecord.has_value()) {
        return makeBorrowResult(BorrowStatus::AlreadyBorrowed, "该读者已借阅此图书。");
    }

    // 更新读者当前借阅列表
    bool readerUpdated = false;
    bool readerUpdateResult = m_library.updateReader(readerId, [&](Reader& reader) {
        if (reader.addCurrentBorrow(isbn)) {
            readerUpdated = true;
        }
        });
    if (!readerUpdateResult || !readerUpdated) {
        return makeBorrowResult(BorrowStatus::Failed, "更新读者信息失败。");
    }

    // 减少图书库存
    bool stockDeducted = false;
    bool bookUpdateResult = m_library.updateBook(isbn, [&](Book& book) {
        stockDeducted = book.decreaseStock(1);
        if (!stockDeducted) {
            throw std::runtime_error("库存不足。");
        }
        });

    // 如果库存减少失败，回滚读者更新
    if (!bookUpdateResult || !stockDeducted) {
        m_library.updateReader(readerId, [&](Reader& reader) {
            reader.removeCurrentBorrow(isbn);
            });
        return makeBorrowResult(BorrowStatus::BookOutOfStock, "图书库存不足。");
    }

    // 创建借阅记录
    Date borrowDate = DateUtil::today();
    Date dueDate = DateUtil::addDays(borrowDate, m_defaultBorrowDays);
    std::string recordId = m_library.generateBorrowRecordId();
    int borrowHour = DateUtil::currentHour();
    BorrowRecord record(recordId, readerId, isbn, borrowDate, dueDate, borrowHour);

    // 保存借阅记录
    if (!m_library.addBorrowRecord(record)) {
        // 失败时回滚所有更改
        m_library.updateReader(readerId, [&](Reader& reader) {
            reader.removeCurrentBorrow(isbn);
            });
        m_library.updateBook(isbn, [&](Book& book) {
            book.increaseStock(1);
            });
        return makeBorrowResult(BorrowStatus::Failed, "生成借阅记录失败。");
    }

    // 返回成功的借阅结果
    BorrowRecord* storedRecord = m_library.getBorrowRecord(recordId);
    return makeBorrowResult(BorrowStatus::Success, "借书成功。", storedRecord);
}

// 检查借阅资格
bool BorrowService::checkBorrowEligibility(const std::string& readerId, const std::string& isbn) {
    const Reader* reader = m_library.getReader(readerId);
    if (reader == nullptr) {
        return false;
    }

    // 检查读者状态
    if (!reader->isActive() || reader->isSuspended()) {
        return false;
    }
    if (reader->getCurrentBorrowCount() >= reader->getMaxBorrow()) {
        return false;
    }

    // 检查图书状态
    const Book* book = m_library.getBook(isbn);
    if (book == nullptr || book->getStock() <= 0) {
        return false;
    }

    // 检查是否已借阅此书
    auto activeRecord = findActiveBorrowRecord(readerId, isbn);
    if (activeRecord.has_value()) {
        return false;
    }
    return true;
}

// 预约图书
bool BorrowService::reserveBook(const std::string& readerId, const std::string& isbn) {
    bool added = false;
    bool ok = m_library.updateReader(readerId, [&](Reader& reader) {
        reader.addReservation(isbn);
        added = true;
        });
    return ok && added;
}

// ==== 还书相关 =============================================================

// 归还图书
ReturnResult BorrowService::returnBook(const std::string& readerId, const std::string& isbn) {
    // 检查读者和图书是否存在
    Reader* readerPtr = m_library.getReader(readerId);
    if (readerPtr == nullptr) {
        return makeReturnResult(ReturnStatus::ReaderNotFound, "读者不存在。");
    }

    Book* bookPtr = m_library.getBook(isbn);
    if (bookPtr == nullptr) {
        return makeReturnResult(ReturnStatus::BookNotFound, "图书不存在。");
    }

    // 查找活跃的借阅记录
    auto activeRecordOpt = findActiveBorrowRecord(readerId, isbn);
    if (!activeRecordOpt.has_value()) {
        return makeReturnResult(ReturnStatus::RecordNotFound, "未找到借阅记录。");
    }

    BorrowRecord* recordPtr = activeRecordOpt.value();
    if (recordPtr->isReturned()) {
        return makeReturnResult(ReturnStatus::AlreadyReturned, "该图书已归还。");
    }

    // 更新归还日期和计算罚金
    Date returnDate = DateUtil::today();
    double fine = calculateOverdueFine(*recordPtr);

    bool recordUpdated = m_library.updateBorrowRecord(recordPtr->getRecordId(), [&](BorrowRecord& record) {
        record.setReturnDate(returnDate);
        record.setFine(fine);
        });

    if (!recordUpdated) {
        return makeReturnResult(ReturnStatus::Failed, "更新借阅记录失败。");
    }

    // 更新图书库存
    m_library.updateBook(isbn, [&](Book& book) {
        book.increaseStock(1);
        });

    // 更新读者信息
    m_library.updateReader(readerId, [&](Reader& reader) {
        reader.removeCurrentBorrow(isbn);
        reader.updateLastActiveDate(returnDate);
        });

    // 返回归还结果
    BorrowRecord* updatedRecord = m_library.getBorrowRecord(recordPtr->getRecordId());
    return makeReturnResult(ReturnStatus::Success, "还书成功。", fine, updatedRecord);
}

// 计算逾期罚金
double BorrowService::calculateOverdueFine(const BorrowRecord& record) const {
    Date today = DateUtil::today();
    int overdueDays = record.getOverdueDays(today);
    if (overdueDays <= 0) {
        return 0.0;
    }
    return overdueDays * m_finePerDay;  // 按天计算罚金
}

// 续借图书
bool BorrowService::renewBook(const std::string& readerId, const std::string& isbn) {
    // 查找活跃的借阅记录
    auto activeRecordOpt = findActiveBorrowRecord(readerId, isbn);
    if (!activeRecordOpt.has_value()) {
        return false;
    }

    BorrowRecord* recordPtr = activeRecordOpt.value();
    // 检查续借次数限制
    if (recordPtr->getRenewCount() >= m_maxRenewals) {
        return false;
    }
    if (recordPtr->isReturned()) {
        return false;
    }

    // 延长应还日期
    Date newDueDate = DateUtil::addDays(recordPtr->getDueDate(), m_defaultBorrowDays);

    return m_library.updateBorrowRecord(recordPtr->getRecordId(), [&](BorrowRecord& record) {
        record.setDueDate(newDueDate);
        record.incrementRenewCount();
        });
}

// ==== 借阅记录查询 =========================================================

// 获取读者借阅历史
std::vector<BorrowRecord> BorrowService::getReaderBorrowHistory(const std::string& readerId) {
    return m_library.getBorrowRecordsByReader(readerId);
}

// 获取图书借阅历史
std::vector<BorrowRecord> BorrowService::getBookBorrowHistory(const std::string& isbn) {
    return m_library.getBorrowRecordsByISBN(isbn);
}

// 获取读者当前借阅
std::vector<BorrowRecord> BorrowService::getCurrentBorrows(const std::string& readerId) {
    std::vector<BorrowRecord> result;
    auto records = m_library.getBorrowRecordsByReader(readerId);
    for (const auto& record : records) {
        if (!record.isReturned()) {  // 只返回未归还的记录
            result.push_back(record);
        }
    }
    return result;
}

// 获取逾期记录
std::vector<BorrowRecord> BorrowService::getOverdueRecords() {
    std::vector<BorrowRecord> result;
    auto records = m_library.getAllBorrowRecords();
    Date today = DateUtil::today();
    for (const auto& record : records) {
        if (!record.isReturned() && record.isOverdue(today)) {  // 未归还且已逾期
            result.push_back(record);
        }
    }
    return result;
}

// 按日期范围获取记录
std::vector<BorrowRecord> BorrowService::getRecordsByDateRange(Date start, Date end) {
    if (end < start) {
        std::swap(start, end);  // 确保开始日期早于结束日期
    }
    std::vector<BorrowRecord> result;
    auto records = m_library.getAllBorrowRecords();
    for (const auto& record : records) {
        const Date& borrowDate = record.getBorrowDate();
        if (borrowDate >= start && borrowDate <= end) {
            result.push_back(record);
        }
    }
    return result;
}

// 获取所有借阅记录
std::vector<BorrowRecord> BorrowService::getAllBorrowRecords() const {
    return m_library.getAllBorrowRecords();
}

// ==== 统计功能 =============================================================

// 获取每日统计
BorrowStatistics BorrowService::getDailyStatistics(Date date) {
    BorrowStatistics stats{};
    auto records = m_library.getAllBorrowRecords();
    for (const auto& record : records) {
        if (record.getBorrowDate() == date) {
            ++stats.totalBorrowed;  // 当日借阅数
        }
        if (record.getReturnDate().has_value() && record.getReturnDate().value() == date) {
            ++stats.totalReturned;  // 当日归还数
            stats.totalFines += record.getFine();  // 累计罚金
            if (record.getReturnDate().value() > record.getDueDate()) {
                ++stats.overdueCount;  // 逾期归还数
            }
        }
    }
    return stats;
}

// 获取月度统计
BorrowStatistics BorrowService::getMonthlyStatistics(int year, int month) {
    BorrowStatistics stats{};
    auto records = m_library.getAllBorrowRecords();
    for (const auto& record : records) {
        const Date& borrowDate = record.getBorrowDate();
        if (borrowDate.year == year && borrowDate.month == month) {
            ++stats.totalBorrowed;  // 当月借阅数
        }
        if (record.getReturnDate().has_value()) {
            const Date& returnDate = record.getReturnDate().value();
            if (returnDate.year == year && returnDate.month == month) {
                ++stats.totalReturned;  // 当月归还数
                stats.totalFines += record.getFine();  // 累计罚金
                if (returnDate > record.getDueDate()) {
                    ++stats.overdueCount;  // 逾期归还数
                }
            }
        }
    }
    return stats;
}

// 获取热门图书
std::map<std::string, int> BorrowService::getPopularBooks(int limit) {
    std::unordered_map<std::string, int> counter;
    auto records = m_library.getAllBorrowRecords();
    for (const auto& record : records) {
        counter[record.getISBN()]++;  // 统计每本书的借阅次数
    }

    // 按借阅次数排序
    std::vector<std::pair<std::string, int>> vec(counter.begin(), counter.end());
    std::sort(vec.begin(), vec.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;  // 次数相同按ISBN排序
        }
        return lhs.second > rhs.second;  // 按次数降序
        });

    // 限制返回数量
    if (limit > 0 && static_cast<std::size_t>(limit) < vec.size()) {
        vec.resize(static_cast<std::size_t>(limit));
    }

    // 转换为map返回
    std::map<std::string, int> result;
    for (const auto& item : vec) {
        result[item.first] = item.second;
    }
    return result;
}

// ==== 配置接口 =============================================================

// 设置每日罚金
void BorrowService::setFinePerDay(double fine) {
    if (fine < 0.0) {
        fine = 0.0;
    }
    m_finePerDay = fine;
}

// 设置默认借阅天数
void BorrowService::setDefaultBorrowDays(int days) {
    if (days <= 0) {
        days = 14;
    }
    m_defaultBorrowDays = days;
}

// 设置最大续借次数
void BorrowService::setMaxRenewals(int count) {
    if (count < 0) {
        count = 0;
    }
    m_maxRenewals = count;
}

// ==== 内部工具 =============================================================

// 查找活跃的借阅记录
std::optional<BorrowRecord*> BorrowService::findActiveBorrowRecord(const std::string& readerId,
    const std::string& isbn) const {
    auto records = m_library.getAllBorrowRecords();
    for (auto& record : records) {
        if (record.getReaderId() == readerId && record.getISBN() == isbn && !record.isReturned()) {
            BorrowRecord* stored = m_library.getBorrowRecord(record.getRecordId());
            if (stored != nullptr) {
                return stored;
            }
        }
    }
    return std::nullopt;
}

// 确保读者有借阅资格
bool BorrowService::ensureReaderEligibility(const Reader& reader) {
    return reader.isActive() && !reader.isSuspended();
}

// 释放过期的账户暂停
void BorrowService::releaseSuspensionIfExpired(Reader& reader) {
    if (reader.isSuspended()) {
        if (DateUtil::today() >= reader.getSuspensionLiftDate()) {
            reader.clearSuspension();
        }
    }
}

// 创建借阅结果对象
BorrowResult BorrowService::makeBorrowResult(BorrowStatus status, const std::string& message,
    BorrowRecord* recordPtr) const {
    BorrowResult result;
    result.status = status;
    result.message = message;
    if (recordPtr != nullptr) {
        result.record = std::make_shared<BorrowRecord>(*recordPtr);  // 使用智能指针
    }
    return result;
}

// 创建归还结果对象
ReturnResult BorrowService::makeReturnResult(ReturnStatus status, const std::string& message,
    double fine, BorrowRecord* recordPtr) const {
    ReturnResult result;
    result.status = status;
    result.message = message;
    result.fine = fine;
    if (recordPtr != nullptr) {
        result.record = std::make_shared<BorrowRecord>(*recordPtr);  // 使用智能指针
    }
    return result;
}
