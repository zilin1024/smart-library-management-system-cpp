#include "SearchService.h"
#include "DateUtil.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace {

    // 辅助函数：截取向量的前 limit 个元素
    template <typename T>
    std::vector<T> limitVector(const std::vector<T>& source, int limit) {
        if (limit <= 0 || static_cast<size_t>(limit) >= source.size()) {
            return source;
        }
        return std::vector<T>(source.begin(), source.begin() + static_cast<size_t>(limit));
    }

    // 辅助函数：向向量中插入元素，如果已存在则不插入（去重）
    void insertUnique(std::vector<std::string>& container, const std::string& value) {
        if (std::find(container.begin(), container.end(), value) == container.end()) {
            container.push_back(value);
        }
    }

} // namespace

// 构造函数：注入 BookService 和 ReaderService 依赖
SearchService::SearchService(BookService& bookService, ReaderService& readerService)
    : m_bookService(bookService),
    m_readerService(readerService) {}

// ==== 图书搜索代理 =========================================================

std::vector<Book> SearchService::searchBooksByKeyword(const std::string& keyword) {
    return m_bookService.searchByKeyword(keyword);
}

std::vector<Book> SearchService::searchBooksByCriteria(const SearchCriteria& criteria) {
    return m_bookService.searchByMultipleCriteria(criteria);
}

std::vector<Book> SearchService::fuzzySearchBooks(const std::string& query, int tolerance) {
    return m_bookService.fuzzySearch(query, tolerance);
}

// ==== 读者搜索代理与扩展 ===================================================

std::vector<Reader> SearchService::searchReadersByName(const std::string& name) {
    return m_readerService.searchReadersByName(name);
}

// 查找借阅过（当前在借或历史借阅）指定 ISBN 的所有读者
std::vector<Reader> SearchService::searchReadersByBorrowedISBN(const std::string& isbn) {
    std::vector<Reader> result;
    auto readers = gatherActiveReaders();

    for (const auto& reader : readers) {
        const auto& current = reader.getCurrentBorrowedIsbns();
        const auto& history = reader.getBorrowHistory();

        // 检查当前借阅或历史记录中是否包含该 ISBN
        bool hasBorrowed = std::find(current.begin(), current.end(), isbn) != current.end() ||
            std::find(history.begin(), history.end(), isbn) != history.end();

        if (hasBorrowed) {
            result.push_back(reader);
        }
    }
    return result;
}

// 查找在指定日期范围内有活跃记录的读者
std::vector<Reader> SearchService::searchReadersByActivity(Date start, Date end) {
    if (end < start) {
        std::swap(start, end);
    }

    std::vector<Reader> result;
    auto readers = gatherActiveReaders();
    for (const auto& reader : readers) {
        const Date& lastActive = reader.getLastActiveDate();
        if (lastActive >= start && lastActive <= end) {
            result.push_back(reader);
        }
    }
    return result;
}

// ==== 高级统计与推荐搜索 ===================================================

// 获取与指定图书经常被一起借阅的其他图书（基于共现统计）
std::vector<Book> SearchService::getBooksBorrowedTogether(const std::string& isbn, int limit) {
    std::unordered_map<std::string, int> counter;
    auto readers = gatherActiveReaders();

    // 遍历所有读者，找出借过目标 ISBN 的人
    for (const auto& reader : readers) {
        const auto& history = reader.getBorrowHistory();
        if (std::find(history.begin(), history.end(), isbn) == history.end()) {
            continue;
        }

        // 统计这些人借阅过的其他书籍
        for (const auto& otherIsbn : history) {
            if (StringUtil::equalsIgnoreCase(otherIsbn, isbn)) {
                continue; // 排除自身
            }
            counter[otherIsbn]++;
        }
    }

    // 将统计结果转换为 pair 向量以便排序
    std::vector<std::pair<std::string, int>> ranking(counter.begin(), counter.end());
    std::sort(ranking.begin(), ranking.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;
        }
        return lhs.second > rhs.second; // 按出现次数降序
        });

    // 获取对应的图书对象
    std::vector<Book> recommendations;
    for (const auto& [otherIsbn, _count] : ranking) {
        auto books = m_bookService.searchByISBN(otherIsbn);
        if (!books.empty()) {
            recommendations.push_back(books.front());
        }
        if (limit > 0 && static_cast<int>(recommendations.size()) >= limit) {
            break;
        }
    }
    return recommendations;
}

// 获取借阅频率最高的 ISBN 列表（热门借阅）
std::vector<std::string> SearchService::getFrequentlyBorrowedISBNs(int limit) {
    std::unordered_map<std::string, int> counter;
    auto readers = gatherActiveReaders();

    // 统计所有读者的所有借阅历史
    for (const auto& reader : readers) {
        for (const auto& isbn : reader.getBorrowHistory()) {
            counter[StringUtil::toLower(isbn)]++;
        }
    }

    std::vector<std::pair<std::string, int>> ranking(counter.begin(), counter.end());
    std::sort(ranking.begin(), ranking.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;
        }
        return lhs.second > rhs.second; // 降序
        });

    std::vector<std::string> result;
    result.reserve(ranking.size());
    for (const auto& [isbn, _count] : ranking) {
        result.push_back(isbn);
    }
    return limitVector(result, limit);
}

// ==== 自动补全建议 =========================================================

// 获取书名搜索建议（前缀匹配）
std::vector<std::string> SearchService::getBookTitleSuggestions(const std::string& prefix, int limit) {
    auto books = m_bookService.searchByTitle(prefix);
    std::vector<std::string> suggestions;
    suggestions.reserve(books.size());

    for (const auto& book : books) {
        // 再次确认前缀匹配（BookService可能返回模糊匹配结果）
        if (StringUtil::startsWith(book.getTitle(), prefix, true)) {
            insertUnique(suggestions, book.getTitle());
        }
    }

    std::sort(suggestions.begin(), suggestions.end());
    return limitVector(suggestions, limit);
}

// 获取作者搜索建议（前缀匹配）
std::vector<std::string> SearchService::getAuthorSuggestions(const std::string& prefix, int limit) {
    auto books = m_bookService.searchByAuthor(prefix);
    std::vector<std::string> suggestions;

    for (const auto& book : books) {
        if (StringUtil::startsWith(book.getAuthor(), prefix, true)) {
            insertUnique(suggestions, book.getAuthor());
        }
    }

    std::sort(suggestions.begin(), suggestions.end());
    return limitVector(suggestions, limit);
}

// 私有辅助：获取所有活跃读者
std::vector<Reader> SearchService::gatherActiveReaders() const {
    auto readers = m_readerService.getReadersByBorrowCount(0);
    std::vector<Reader> activeReaders;
    activeReaders.reserve(readers.size());

    for (const auto& reader : readers) {
        if (reader.isActive()) {
            activeReaders.push_back(reader);
        }
    }
    return activeReaders;
}
