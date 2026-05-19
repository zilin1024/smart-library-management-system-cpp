#ifndef BOOK_SERVICE_H
#define BOOK_SERVICE_H

#include "Book.h"
#include "LibraryManager.h"
#include "StringUtil.h"

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @struct BookUpdate
 * @brief 图书信息更新结构体，支持部分字段更新。
 */
struct BookUpdate {
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> publisher;
    std::optional<std::string> category;
    std::optional<int> publishYear;
    std::optional<int> stock;
    std::optional<std::vector<std::string>> tags;
};

/**
 * @struct SearchCriteria
 * @brief 多字段组合查询条件。
 */
struct SearchCriteria {
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> category;
    std::optional<std::string> publisher;
    std::optional<std::string> isbn;
    std::optional<int> year;
    std::optional<std::pair<int, int>> yearRange;
    std::optional<std::pair<int, int>> stockRange;
    std::optional<bool> inStockOnly;
    std::optional<bool> outOfStockOnly;

    [[nodiscard]] bool hasCriteria() const {
        return title.has_value() || author.has_value() || category.has_value() ||
            publisher.has_value() || isbn.has_value() || year.has_value() ||
            yearRange.has_value() || stockRange.has_value() ||
            inStockOnly.has_value() || outOfStockOnly.has_value();
    }
};

/**
 * @class BookService
 * @brief 图书服务层，封装所有图书相关的业务逻辑。
 */
class BookService {
public:
    explicit BookService(LibraryManager& manager);

    // ===== 1. 添加图书 =====
    bool addBook(const Book& book);
    bool batchAddBooks(const std::vector<Book>& books);

    // ===== 2. 删除图书 =====
    bool deleteBook(const std::string& isbn);
    bool deleteBookByTitle(const std::string& title);

    // ===== 3. 修改图书信息 =====
    bool updateBookInfo(const std::string& isbn, const BookUpdate& update);
    bool updateStock(const std::string& isbn, int newStock);

    // ===== 4. 查询图书 =====
    std::vector<Book> searchByTitle(const std::string& title);
    std::vector<Book> searchByAuthor(const std::string& author);
    std::vector<Book> searchByCategory(const std::string& category);
    std::vector<Book> searchByPublisher(const std::string& publisher);
    std::vector<Book> searchByISBN(const std::string& isbn);
    std::vector<Book> searchByISBNPartial(const std::string& isbnPart);
    std::vector<Book> searchByYear(int year);
    std::vector<Book> searchByYearRange(int startYear, int endYear);
    std::vector<Book> searchByExactYear(int year);
    std::vector<Book> searchByStock(int stock);
    std::vector<Book> searchByStockRange(int minStock, int maxStock);
    std::vector<Book> searchInStock();
    std::vector<Book> searchOutOfStock();
    std::vector<Book> searchLowStock(int threshold = 5);
    std::vector<Book> searchByMultipleCriteria(const SearchCriteria& criteria);
    std::vector<Book> searchByKeyword(const std::string& keyword);
    std::vector<Book> fuzzySearch(const std::string& query, int tolerance = 2);

    // ===== 5. 统计功能 =====
    int getTotalBookCount() const;
    std::map<std::string, int> getCategoryDistribution() const;
    std::vector<Book> getLowStockBooks(int threshold = 5) const;
    std::map<int, int> getYearDistribution() const;
    std::map<std::string, int> getPublisherDistribution() const;

    // ===== 6. 库存管理 =====
    bool increaseStock(const std::string& isbn, int amount);
    bool decreaseStock(const std::string& isbn, int amount);
    bool setStock(const std::string& isbn, int newStock);

private:
    LibraryManager& m_library;
    std::unordered_map<std::string, std::set<std::string>> m_titleIndex;
    std::unordered_map<std::string, std::set<std::string>> m_authorIndex;
    std::unordered_map<std::string, std::set<std::string>> m_categoryIndex;
    std::unordered_map<std::string, std::set<std::string>> m_publisherIndex;
    std::map<int, std::set<std::string>> m_yearIndex;
    std::map<int, std::set<std::string>> m_stockIndex;

    void indexAllBooks();
    void indexBook(const Book& book);
    void removeFromIndex(const Book& book);
    void rebuildStockIndex();

    std::vector<Book> gatherBooksByISBNs(const std::set<std::string>& isbns) const;
    std::vector<Book> filterBooks(const std::function<bool(const Book&)>& predicate) const;

    std::set<std::string> intersectISBNs(const std::vector<std::set<std::string>>& sets) const;
};

#endif // BOOK_SERVICE_H
