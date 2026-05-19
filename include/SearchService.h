#ifndef SEARCH_SERVICE_H
#define SEARCH_SERVICE_H

#include "BookService.h"
#include "ReaderService.h"
#include "StringUtil.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

/**
 * @class SearchService
 * @brief 服务层复合搜索模块，整合图书与读者的查询能力，提供关键词、组合条件与统计搜索功能。
 */
class SearchService {
public:
    SearchService(BookService& bookService, ReaderService& readerService);

    // ==== 图书搜索 ====
    std::vector<Book> searchBooksByKeyword(const std::string& keyword);
    std::vector<Book> searchBooksByCriteria(const SearchCriteria& criteria);
    std::vector<Book> fuzzySearchBooks(const std::string& query, int tolerance = 2);

    // ==== 读者搜索 ====
    std::vector<Reader> searchReadersByName(const std::string& name);
    std::vector<Reader> searchReadersByBorrowedISBN(const std::string& isbn);
    std::vector<Reader> searchReadersByActivity(Date start, Date end);

    // ==== 推荐辅助 ====
    std::vector<Book> getBooksBorrowedTogether(const std::string& isbn, int limit = 10);
    std::vector<std::string> getFrequentlyBorrowedISBNs(int limit = 10);

    // ==== 搜索建议 ====
    std::vector<std::string> getBookTitleSuggestions(const std::string& prefix, int limit = 10);
    std::vector<std::string> getAuthorSuggestions(const std::string& prefix, int limit = 10);

private:
    BookService& m_bookService;
    ReaderService& m_readerService;

    std::vector<Reader> gatherActiveReaders() const;
};

#endif // SEARCH_SERVICE_H
