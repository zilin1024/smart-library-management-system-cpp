#ifndef LIBRARY_MANAGER_H
#define LIBRARY_MANAGER_H

#include "Book.h"
#include "BorrowRecord.h"
#include "Reader.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class LibraryManager
 * @brief 基础层数据管理中心，负责维护图书、读者、借阅记录等核心数据。
 *
 * LibraryManager 为各服务层提供统一的数据访问接口，屏蔽底层容器实现细节，
 * 确保数据的一致性与可维护性。
 */
class LibraryManager {
public:
    LibraryManager();

    // ==== 图书管理 ====
    bool addBook(const Book& book);
    bool removeBook(const std::string& isbn);
    Book* getBook(const std::string& isbn);
    const Book* getBook(const std::string& isbn) const;
    std::vector<Book> getAllBooks() const;
    bool updateBook(const std::string& isbn, const std::function<void(Book&)>& modifier);

    // ==== 读者管理 ====
    bool addReader(const Reader& reader);
    bool removeReader(const std::string& readerId);
    Reader* getReader(const std::string& readerId);
    const Reader* getReader(const std::string& readerId) const;
    std::vector<Reader> getAllReaders() const;
    bool updateReader(const std::string& readerId, const std::function<void(Reader&)>& modifier);

    // ==== 借阅记录管理 ====
    bool addBorrowRecord(const BorrowRecord& record);
    bool updateBorrowRecord(const std::string& recordId, const std::function<void(BorrowRecord&)>& modifier);
    BorrowRecord* getBorrowRecord(const std::string& recordId);
    const BorrowRecord* getBorrowRecord(const std::string& recordId) const;
    std::vector<BorrowRecord> getBorrowRecordsByReader(const std::string& readerId) const;
    std::vector<BorrowRecord> getBorrowRecordsByISBN(const std::string& isbn) const;
    std::vector<BorrowRecord> getAllBorrowRecords() const;
    bool removeBorrowRecord(const std::string& recordId);

    // ==== ID 生成器 ====
    std::string generateBorrowRecordId();
    std::string generateNotificationId();
    std::string generateReservationId();

    // ==== 数据统计 ====
    int getTotalBookCount() const;
    int getTotalReaderCount() const;
    int getTotalBorrowRecordCount() const;

    // ==== 数据重置 ====
    void clearAll();

private:
    std::unordered_map<std::string, Book> m_books;           // ISBN -> Book
    std::unordered_map<std::string, Reader> m_readers;       // readerId -> Reader
    std::unordered_map<std::string, BorrowRecord> m_borrowRecords; // recordId -> BorrowRecord
    long long m_recordSequence{ 0 };        // 借阅记录ID序列
    long long m_notificationSequence{ 0 };  // 通知ID序列  
    long long m_reservationSequence{ 0 };   // 预约ID序列
};

#endif // LIBRARY_MANAGER_H
