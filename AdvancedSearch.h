#ifndef ADVANCED_SEARCH_H
#define ADVANCED_SEARCH_H

#include "BookService.h"
#include "ReaderService.h"
#include "StringUtil.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @struct SearchQuery
 * @brief 高级搜索查询对象。
 */
struct SearchQuery {
    std::optional<std::string> keyword;
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> category;
    std::optional<std::string> publisher;
    std::optional<int> year;
};

/**
 * @struct FilterCriteria
 * @brief 高级过滤条件。
 */
struct FilterCriteria {
    std::optional<std::set<std::string>> categories;
    std::optional<std::pair<int, int>> yearRange;
    std::optional<std::pair<int, int>> stockRange;
    std::optional<bool> inStockOnly;
};

/**
 * @class AdvancedSearch
 * @brief 高级搜索引擎，提供多字段、模糊、语义与推荐相关搜索功能。
 */
class AdvancedSearch {
public:
    AdvancedSearch(BookService& bookService, ReaderService& readerService);

    // === 基础搜索 ===
    std::vector<Book> searchByKeyword(const std::string& keyword);
    std::vector<Book> searchByPhrase(const std::string& phrase);

    // === 高级搜索 ===
    std::vector<Book> multiFieldSearch(const SearchQuery& query);
    std::vector<Book> advancedFilterSearch(const FilterCriteria& filters);

    // === 智能搜索 ===
    std::vector<Book> semanticSearch(const std::string& query);
    std::vector<Book> fuzzySearch(const std::string& query, int tolerance = 2);
    std::vector<Book> synonymSearch(const std::string& query);

    // === 搜索优化 ===
    std::vector<std::string> getSearchSuggestions(const std::string& prefix);
    std::vector<std::string> getPopularSearches(int limit = 10);
    void saveSearchHistory(const std::string& readerId, const std::string& query);

    // === 全文检索 ===
    bool buildFullTextIndex();
    std::vector<Book> fullTextSearch(const std::string& query);
    double getSearchRelevanceScore(const Book& book, const std::string& query);

private:
    BookService& m_bookService;
    ReaderService& m_readerService;

    std::unordered_map<std::string, int> m_searchFrequency;
    std::unordered_map<std::string, std::vector<std::string>> m_readerHistory;
    std::unordered_map<std::string, std::set<std::string>> m_invertedIndex;

    void updateSearchFrequency(const std::string& query);
    std::set<std::string> tokenize(const std::string& text) const;
    double computeTfIdfScore(const Book& book, const std::string& term) const;
};

#endif // ADVANCED_SEARCH_H
