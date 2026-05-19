#include "LibraryManager.h"
#include <algorithm>
#include <chrono>
#include <sstream>

// === 构造函数 ==============================================================

LibraryManager::LibraryManager()
    : m_recordSequence(0),
    m_notificationSequence(0),
    m_reservationSequence(0) {}

// === 图书管理 ==============================================================

// 添加图书：若 ISBN 为空或已存在则返回 false
bool LibraryManager::addBook(const Book& book) {
    const auto& isbn = book.getISBN();
    if (isbn.empty() || m_books.contains(isbn)) {
        return false;
    }
    m_books.emplace(isbn, book);
    return true;
}

// 移除图书
bool LibraryManager::removeBook(const std::string& isbn) {
    return m_books.erase(isbn) > 0;
}

// 获取图书指针（可变），不存在返回 nullptr
Book* LibraryManager::getBook(const std::string& isbn) {
    auto it = m_books.find(isbn);
    if (it == m_books.end()) {
        return nullptr;
    }
    return &it->second;
}

// 获取图书指针（只读），不存在返回 nullptr
const Book* LibraryManager::getBook(const std::string& isbn) const {
    auto it = m_books.find(isbn);
    if (it == m_books.end()) {
        return nullptr;
    }
    return &it->second;
}

// 获取所有图书列表
std::vector<Book> LibraryManager::getAllBooks() const {
    std::vector<Book> books;
    books.reserve(m_books.size());
    for (const auto& [isbn, book] : m_books) {
        books.push_back(book);
    }
    return books;
}

// 更新图书信息：通过回调函数安全修改
bool LibraryManager::updateBook(const std::string& isbn, const std::function<void(Book&)>& modifier) {
    auto it = m_books.find(isbn);
    if (it == m_books.end()) {
        return false;
    }
    modifier(it->second);
    return true;
}

// === 读者管理 ==============================================================

// 添加读者：若 ID 重复则返回 false
bool LibraryManager::addReader(const Reader& reader) {
    const auto& readerId = reader.getReaderId();
    if (readerId.empty() || m_readers.contains(readerId)) {
        return false;
    }
    m_readers.emplace(readerId, reader);
    return true;
}

// 移除读者
bool LibraryManager::removeReader(const std::string& readerId) {
    return m_readers.erase(readerId) > 0;
}

// 获取读者指针（可变）
Reader* LibraryManager::getReader(const std::string& readerId) {
    auto it = m_readers.find(readerId);
    if (it == m_readers.end()) {
        return nullptr;
    }
    return &it->second;
}

// 获取读者指针（只读）
const Reader* LibraryManager::getReader(const std::string& readerId) const {
    auto it = m_readers.find(readerId);
    if (it == m_readers.end()) {
        return nullptr;
    }
    return &it->second;
}

// 获取所有读者列表
std::vector<Reader> LibraryManager::getAllReaders() const {
    std::vector<Reader> readers;
    readers.reserve(m_readers.size());
    for (const auto& [readerId, reader] : m_readers) {
        readers.push_back(reader);
    }
    return readers;
}

// 更新读者信息
bool LibraryManager::updateReader(const std::string& readerId, const std::function<void(Reader&)>& modifier) {
    auto it = m_readers.find(readerId);
    if (it == m_readers.end()) {
        return false;
    }
    modifier(it->second);
    return true;
}

// === 借阅记录管理 ==========================================================

// 添加借阅记录
bool LibraryManager::addBorrowRecord(const BorrowRecord& record) {
    const auto& recordId = record.getRecordId();
    if (recordId.empty() || m_borrowRecords.contains(recordId)) {
        return false;
    }
    m_borrowRecords.emplace(recordId, record);

    // 同步序号，防止后续生成重复 ID
    // 解析类似 "BR-100" 的格式，确保内部计数器 m_recordSequence 大于当前最大值
    constexpr std::string_view kPrefix = "BR-";
    if (recordId.starts_with(kPrefix)) {
        try {
            long long num = std::stoll(recordId.substr(kPrefix.size()));
            if (num > m_recordSequence) {
                m_recordSequence = num;
            }
        }
        catch (...) {
            // 非标准格式，忽略
        }
    }
    return true;
}

// 更新借阅记录
bool LibraryManager::updateBorrowRecord(const std::string& recordId, const std::function<void(BorrowRecord&)>& modifier) {
    auto it = m_borrowRecords.find(recordId);
    if (it == m_borrowRecords.end()) {
        return false;
    }
    modifier(it->second);
    return true;
}

// 获取借阅记录（可变）
BorrowRecord* LibraryManager::getBorrowRecord(const std::string& recordId) {
    auto it = m_borrowRecords.find(recordId);
    if (it == m_borrowRecords.end()) {
        return nullptr;
    }
    return &it->second;
}

// 获取借阅记录（只读）
const BorrowRecord* LibraryManager::getBorrowRecord(const std::string& recordId) const {
    auto it = m_borrowRecords.find(recordId);
    if (it == m_borrowRecords.end()) {
        return nullptr;
    }
    return &it->second;
}

// 获取指定读者的所有借阅记录
std::vector<BorrowRecord> LibraryManager::getBorrowRecordsByReader(const std::string& readerId) const {
    std::vector<BorrowRecord> records;
    for (const auto& [id, record] : m_borrowRecords) {
        if (record.getReaderId() == readerId) {
            records.push_back(record);
        }
    }
    return records;
}

// 获取指定图书的所有借阅记录
std::vector<BorrowRecord> LibraryManager::getBorrowRecordsByISBN(const std::string& isbn) const {
    std::vector<BorrowRecord> records;
    for (const auto& [id, record] : m_borrowRecords) {
        if (record.getISBN() == isbn) {
            records.push_back(record);
        }
    }
    return records;
}

// 获取所有借阅记录
std::vector<BorrowRecord> LibraryManager::getAllBorrowRecords() const {
    std::vector<BorrowRecord> records;
    records.reserve(m_borrowRecords.size());
    for (const auto& [id, record] : m_borrowRecords) {
        records.push_back(record);
    }
    return records;
}

// 移除借阅记录
bool LibraryManager::removeBorrowRecord(const std::string& recordId) {
    return m_borrowRecords.erase(recordId) > 0;
}

// === ID 生成器 ============================================================

// 生成唯一的借阅记录ID (格式: BR-Sequence)
std::string LibraryManager::generateBorrowRecordId() {
    ++m_recordSequence;
    std::ostringstream oss;
    oss << "BR-" << m_recordSequence;
    return oss.str();
}

// 生成通知ID (格式: NT-Sequence)
std::string LibraryManager::generateNotificationId() {
    ++m_notificationSequence;
    std::ostringstream oss;
    oss << "NT-" << m_notificationSequence;
    return oss.str();
}

// 生成预约ID (格式: RS-Sequence)
std::string LibraryManager::generateReservationId() {
    ++m_reservationSequence;
    std::ostringstream oss;
    oss << "RS-" << m_reservationSequence;
    return oss.str();
}

// === 数据统计 =============================================================

int LibraryManager::getTotalBookCount() const {
    return static_cast<int>(m_books.size());
}

int LibraryManager::getTotalReaderCount() const {
    return static_cast<int>(m_readers.size());
}

int LibraryManager::getTotalBorrowRecordCount() const {
    return static_cast<int>(m_borrowRecords.size());
}

// === 数据重置 =============================================================

// 清空所有数据并重置计数器
void LibraryManager::clearAll() {
    m_books.clear();
    m_readers.clear();
    m_borrowRecords.clear();
    m_recordSequence = 0;
    m_notificationSequence = 0;
    m_reservationSequence = 0;
}