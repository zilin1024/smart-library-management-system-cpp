#include "BookService.h"
#include <limits>
#include <stdexcept>

namespace {

    // 规范化文本键（转换为小写并去除首尾空格）
    std::string normalizeKey(const std::string& text) {
        return StringUtil::toLower(StringUtil::trim(text));
    }

    // 合并两个集合
    std::set<std::string> unionSets(const std::set<std::string>& a,
        const std::set<std::string>& b) {
        std::set<std::string> result = a;
        result.insert(b.begin(), b.end());
        return result;
    }

    // 计算两个字符串的编辑距离
    int editDistance(const std::string& lhs, const std::string& rhs) {
        const std::string a = StringUtil::toLower(lhs);
        const std::string b = StringUtil::toLower(rhs);

        const size_t m = a.size();
        const size_t n = b.size();

        // 动态规划计算编辑距离
        std::vector<int> prev(n + 1);
        std::vector<int> curr(n + 1);

        for (size_t j = 0; j <= n; ++j) {
            prev[j] = static_cast<int>(j);
        }

        for (size_t i = 1; i <= m; ++i) {
            curr[0] = static_cast<int>(i);
            for (size_t j = 1; j <= n; ++j) {
                int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                curr[j] = std::min({ prev[j] + 1,           // 删除
                                     curr[j - 1] + 1,       // 插入
                                     prev[j - 1] + cost }); // 替换
            }
            std::swap(prev, curr);
        }

        return prev[n];
    }

    // 模糊匹配检查（使用编辑距离）
    bool matchesFuzzy(const Book& book, const std::string& query, int tolerance) {
        if (query.empty()) {
            return false;
        }

        const std::string q = StringUtil::toLower(query);

        // 需要匹配的字段列表
        std::vector<std::string> candidates{
            book.getTitle(),
            book.getAuthor(),
            book.getPublisher(),
            book.getCategory(),
            book.getISBN()
        };

        // 添加标签
        for (const auto& tag : book.getTags()) {
            candidates.push_back(tag);
        }

        // 检查是否有字段在容忍度内匹配
        for (const auto& candidate : candidates) {
            if (editDistance(q, candidate) <= tolerance) {
                return true;
            }
        }
        return false;
    }

} // namespace

// ==== 构造与索引 ============================================================

// 构造函数
BookService::BookService(LibraryManager& manager)
    : m_library(manager) {
    indexAllBooks();
}

// 索引所有图书
void BookService::indexAllBooks() {
    // 清空所有索引
    m_titleIndex.clear();
    m_authorIndex.clear();
    m_categoryIndex.clear();
    m_publisherIndex.clear();
    m_yearIndex.clear();
    m_stockIndex.clear();

    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        indexBook(book);
    }
}

// 索引单本图书
void BookService::indexBook(const Book& book) {
    const std::string isbn = book.getISBN();
    const std::string titleKey = normalizeKey(book.getTitle());
    const std::string authorKey = normalizeKey(book.getAuthor());
    const std::string categoryKey = normalizeKey(book.getCategory());
    const std::string publisherKey = normalizeKey(book.getPublisher());

    // 更新各个索引
    if (!titleKey.empty()) {
        m_titleIndex[titleKey].insert(isbn);
    }
    if (!authorKey.empty()) {
        m_authorIndex[authorKey].insert(isbn);
    }
    if (!categoryKey.empty()) {
        m_categoryIndex[categoryKey].insert(isbn);
    }
    if (!publisherKey.empty()) {
        m_publisherIndex[publisherKey].insert(isbn);
    }

    // 数字索引
    m_yearIndex[book.getPublishYear()].insert(isbn);
    m_stockIndex[book.getStock()].insert(isbn);
}

// 从索引中移除图书
void BookService::removeFromIndex(const Book& book) {
    const std::string isbn = book.getISBN();
    const std::string titleKey = normalizeKey(book.getTitle());
    const std::string authorKey = normalizeKey(book.getAuthor());
    const std::string categoryKey = normalizeKey(book.getCategory());
    const std::string publisherKey = normalizeKey(book.getPublisher());

    // 移除文本索引的辅助lambda
    auto removeKey = [&](auto& index, const auto& key) {
        if (key.empty()) {
            return;
        }
        auto it = index.find(key);
        if (it != index.end()) {
            it->second.erase(isbn);
            if (it->second.empty()) {
                index.erase(it);
            }
        }
        };

    removeKey(m_titleIndex, titleKey);
    removeKey(m_authorIndex, authorKey);
    removeKey(m_categoryIndex, categoryKey);
    removeKey(m_publisherIndex, publisherKey);

    // 移除数字索引的辅助lambda
    auto removeNumeric = [&](auto& index, auto key) {
        auto it = index.find(key);
        if (it != index.end()) {
            it->second.erase(isbn);
            if (it->second.empty()) {
                index.erase(it);
            }
        }
        };

    removeNumeric(m_yearIndex, book.getPublishYear());
    removeNumeric(m_stockIndex, book.getStock());
}

// 重建库存索引
void BookService::rebuildStockIndex() {
    m_stockIndex.clear();
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        m_stockIndex[book.getStock()].insert(book.getISBN());
    }
}

// ==== 内部辅助函数 =============================================================

// 根据ISBN集合收集图书
std::vector<Book> BookService::gatherBooksByISBNs(const std::set<std::string>& isbns) const {
    std::vector<Book> result;
    for (const auto& isbn : isbns) {
        const Book* book = m_library.getBook(isbn);
        if (book != nullptr) {
            result.push_back(*book);
        }
    }
    return result;
}

// 使用谓词过滤图书
std::vector<Book> BookService::filterBooks(const std::function<bool(const Book&)>& predicate) const {
    std::vector<Book> result;
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        if (predicate(book)) {
            result.push_back(book);
        }
    }
    return result;
}

// 求多个ISBN集合的交集
std::set<std::string> BookService::intersectISBNs(const std::vector<std::set<std::string>>& sets) const {
    if (sets.empty()) {
        return {};
    }

    std::set<std::string> intersection = sets.front();
    for (size_t i = 1; i < sets.size(); ++i) {
        std::set<std::string> temp;
        std::set_intersection(intersection.begin(), intersection.end(),
            sets[i].begin(), sets[i].end(),
            std::inserter(temp, temp.begin()));
        intersection = std::move(temp);
        if (intersection.empty()) {
            break;
        }
    }
    return intersection;
}

// ==== 业务接口：添加、删除 ===================================================

// 添加图书
bool BookService::addBook(const Book& book) {
    if (!m_library.addBook(book)) {
        return false;
    }
    const Book* added = m_library.getBook(book.getISBN());
    if (added != nullptr) {
        indexBook(*added);  // 添加到索引
    }
    return true;
}

// 批量添加图书
bool BookService::batchAddBooks(const std::vector<Book>& books) {
    bool success = true;
    for (const auto& book : books) {
        success = addBook(book) && success;
    }
    return success;
}

// 删除图书（根据ISBN）
bool BookService::deleteBook(const std::string& isbn) {
    Book* book = m_library.getBook(isbn);
    if (book == nullptr) {
        return false;
    }
    removeFromIndex(*book);  // 从索引中移除
    return m_library.removeBook(isbn);
}

// 删除图书（根据标题）
bool BookService::deleteBookByTitle(const std::string& title) {
    auto matches = searchByTitle(title);
    bool success = false;
    for (const auto& book : matches) {
        success = deleteBook(book.getISBN()) || success;
    }
    return success;
}

// ==== 业务接口：更新 =======================================================

// 更新图书信息
bool BookService::updateBookInfo(const std::string& isbn, const BookUpdate& update) {
    Book* book = m_library.getBook(isbn);
    if (book == nullptr) {
        return false;
    }

    Book oldCopy = *book;  // 保存旧副本用于索引更新
    bool result = m_library.updateBook(isbn, [&](Book& target) {
        if (update.title.has_value()) {
            target.setTitle(update.title.value());
        }
        if (update.author.has_value()) {
            target.setAuthor(update.author.value());
        }
        if (update.publisher.has_value()) {
            target.setPublisher(update.publisher.value());
        }
        if (update.category.has_value()) {
            target.setCategory(update.category.value());
        }
        if (update.publishYear.has_value()) {
            target.setPublishYear(update.publishYear.value());
        }
        if (update.stock.has_value()) {
            target.setStock(update.stock.value());
        }
        if (update.tags.has_value()) {
            target.setTags(update.tags.value());
        }
        });

    if (!result) {
        return false;
    }

    // 更新索引
    Book* updated = m_library.getBook(isbn);
    if (updated != nullptr) {
        removeFromIndex(oldCopy);
        indexBook(*updated);
    }
    return true;
}

// 更新库存
bool BookService::updateStock(const std::string& isbn, int newStock) {
    if (newStock < 0) {
        return false;
    }
    Book* book = m_library.getBook(isbn);
    if (book == nullptr) {
        return false;
    }

    Book oldCopy = *book;
    bool ok = m_library.updateBook(isbn, [&](Book& target) {
        target.setStock(newStock);
        });

    if (!ok) {
        return false;
    }

    // 更新索引
    Book* updated = m_library.getBook(isbn);
    if (updated != nullptr) {
        removeFromIndex(oldCopy);
        indexBook(*updated);
    }
    return true;
}

// ==== 查询功能 =============================================================

// 按标题搜索（模糊匹配）
std::vector<Book> BookService::searchByTitle(const std::string& title) {
    return filterBooks([&](const Book& book) {
        return StringUtil::containsIgnoreCase(book.getTitle(), title);
        });
}

// 按作者搜索（模糊匹配）
std::vector<Book> BookService::searchByAuthor(const std::string& author) {
    return filterBooks([&](const Book& book) {
        return StringUtil::containsIgnoreCase(book.getAuthor(), author);
        });
}

// 按类别搜索（模糊匹配）
std::vector<Book> BookService::searchByCategory(const std::string& category) {
    return filterBooks([&](const Book& book) {
        return StringUtil::containsIgnoreCase(book.getCategory(), category);
        });
}

// 按出版社搜索（模糊匹配）
std::vector<Book> BookService::searchByPublisher(const std::string& publisher) {
    return filterBooks([&](const Book& book) {
        return StringUtil::containsIgnoreCase(book.getPublisher(), publisher);
        });
}

// 按ISBN精确搜索
std::vector<Book> BookService::searchByISBN(const std::string& isbn) {
    std::vector<Book> result;
    const Book* book = m_library.getBook(isbn);
    if (book != nullptr) {
        result.push_back(*book);
    }
    return result;
}

// 按ISBN部分匹配搜索
std::vector<Book> BookService::searchByISBNPartial(const std::string& isbnPart) {
    return filterBooks([&](const Book& book) {
        return StringUtil::containsIgnoreCase(book.getISBN(), isbnPart);
        });
}

// 按出版年份搜索
std::vector<Book> BookService::searchByYear(int year) {
    auto it = m_yearIndex.find(year);
    if (it == m_yearIndex.end()) {
        return {};
    }
    return gatherBooksByISBNs(it->second);
}

// 按年份范围搜索
std::vector<Book> BookService::searchByYearRange(int startYear, int endYear) {
    if (startYear > endYear) {
        std::swap(startYear, endYear);
    }
    std::set<std::string> isbns;
    for (int year = startYear; year <= endYear; ++year) {
        auto it = m_yearIndex.find(year);
        if (it != m_yearIndex.end()) {
            isbns = unionSets(isbns, it->second);
        }
    }
    return gatherBooksByISBNs(isbns);
}

// 按精确年份搜索（与searchByYear相同）
std::vector<Book> BookService::searchByExactYear(int year) {
    return searchByYear(year);
}

// 按库存数量搜索
std::vector<Book> BookService::searchByStock(int stock) {
    auto it = m_stockIndex.find(stock);
    if (it == m_stockIndex.end()) {
        return {};
    }
    return gatherBooksByISBNs(it->second);
}

// 按库存范围搜索
std::vector<Book> BookService::searchByStockRange(int minStock, int maxStock) {
    if (minStock > maxStock) {
        std::swap(minStock, maxStock);
    }
    std::set<std::string> isbns;
    for (auto it = m_stockIndex.lower_bound(minStock); it != m_stockIndex.end() && it->first <= maxStock; ++it) {
        isbns = unionSets(isbns, it->second);
    }
    return gatherBooksByISBNs(isbns);
}

// 搜索有库存的图书
std::vector<Book> BookService::searchInStock() {
    return filterBooks([&](const Book& book) {
        return book.getStock() > 0;
        });
}

// 搜索无库存的图书
std::vector<Book> BookService::searchOutOfStock() {
    return filterBooks([&](const Book& book) {
        return book.getStock() <= 0;
        });
}

// 搜索低库存的图书（小于等于阈值）
std::vector<Book> BookService::searchLowStock(int threshold) {
    return filterBooks([&](const Book& book) {
        return book.getStock() >= 0 && book.getStock() <= threshold;
        });
}

// 多条件搜索
std::vector<Book> BookService::searchByMultipleCriteria(const SearchCriteria& criteria) {
    if (!criteria.hasCriteria()) {
        return m_library.getAllBooks();
    }

    return filterBooks([&](const Book& book) {
        if (criteria.title.has_value() &&
            !StringUtil::containsIgnoreCase(book.getTitle(), criteria.title.value())) {
            return false;
        }

        if (criteria.author.has_value() &&
            !StringUtil::containsIgnoreCase(book.getAuthor(), criteria.author.value())) {
            return false;
        }

        if (criteria.category.has_value() &&
            !StringUtil::containsIgnoreCase(book.getCategory(), criteria.category.value())) {
            return false;
        }

        if (criteria.publisher.has_value() &&
            !StringUtil::containsIgnoreCase(book.getPublisher(), criteria.publisher.value())) {
            return false;
        }

        if (criteria.isbn.has_value() &&
            !StringUtil::containsIgnoreCase(book.getISBN(), criteria.isbn.value())) {
            return false;
        }

        if (criteria.year.has_value() &&
            book.getPublishYear() != criteria.year.value()) {
            return false;
        }

        if (criteria.yearRange.has_value()) {
            auto [startYear, endYear] = criteria.yearRange.value();
            if (startYear > endYear) {
                std::swap(startYear, endYear);
            }
            if (book.getPublishYear() < startYear || book.getPublishYear() > endYear) {
                return false;
            }
        }

        if (criteria.stockRange.has_value()) {
            auto [minStock, maxStock] = criteria.stockRange.value();
            if (minStock > maxStock) {
                std::swap(minStock, maxStock);
            }
            if (book.getStock() < minStock || book.getStock() > maxStock) {
                return false;
            }
        }

        if (criteria.inStockOnly.has_value() && criteria.inStockOnly.value() && book.getStock() <= 0) {
            return false;
        }

        if (criteria.outOfStockOnly.has_value() && criteria.outOfStockOnly.value() && book.getStock() > 0) {
            return false;
        }

        return true;
        });
}

// 关键词搜索
std::vector<Book> BookService::searchByKeyword(const std::string& keyword) {
    return filterBooks([&](const Book& book) {
        return book.matchesKeyword(keyword);
        });
}

// 模糊搜索
std::vector<Book> BookService::fuzzySearch(const std::string& query, int tolerance) {
    return filterBooks([&](const Book& book) {
        return matchesFuzzy(book, query, tolerance);
        });
}

// ==== 统计功能 =============================================================

// 获取总图书数量
int BookService::getTotalBookCount() const {
    return m_library.getTotalBookCount();
}

// 获取类别分布
std::map<std::string, int> BookService::getCategoryDistribution() const {
    std::map<std::string, int> distribution;
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        distribution[book.getCategory()]++;
    }
    return distribution;
}

// 获取低库存图书列表
std::vector<Book> BookService::getLowStockBooks(int threshold) const {
    std::vector<Book> result;
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        if (book.getStock() >= 0 && book.getStock() <= threshold) {
            result.push_back(book);
        }
    }
    return result;
}

// 获取年份分布
std::map<int, int> BookService::getYearDistribution() const {
    std::map<int, int> distribution;
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        distribution[book.getPublishYear()]++;
    }
    return distribution;
}

// 获取出版社分布
std::map<std::string, int> BookService::getPublisherDistribution() const {
    std::map<std::string, int> distribution;
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        distribution[book.getPublisher()]++;
    }
    return distribution;
}

// ==== 库存管理 =============================================================

// 增加库存
bool BookService::increaseStock(const std::string& isbn, int amount) {
    if (amount < 0) {
        return false;
    }
    Book* book = m_library.getBook(isbn);
    if (book == nullptr) {
        return false;
    }

    Book oldCopy = *book;
    bool success = false;

    bool ok = m_library.updateBook(isbn, [&](Book& target) {
        target.increaseStock(amount);
        success = true;
        });

    if (!ok || !success) {
        return false;
    }

    // 更新索引
    Book* updated = m_library.getBook(isbn);
    if (updated != nullptr) {
        removeFromIndex(oldCopy);
        indexBook(*updated);
    }
    return true;
}

// 减少库存
bool BookService::decreaseStock(const std::string& isbn, int amount) {
    if (amount < 0) {
        return false;
    }
    Book* book = m_library.getBook(isbn);
    if (book == nullptr) {
        return false;
    }

    Book oldCopy = *book;
    bool success = false;

    bool ok = m_library.updateBook(isbn, [&](Book& target) {
        success = target.decreaseStock(amount);
        });

    if (!ok || !success) {
        return false;
    }

    // 更新索引
    Book* updated = m_library.getBook(isbn);
    if (updated != nullptr) {
        removeFromIndex(oldCopy);
        indexBook(*updated);
    }
    return true;
}

// 设置库存
bool BookService::setStock(const std::string& isbn, int newStock) {
    return updateStock(isbn, newStock);
}
