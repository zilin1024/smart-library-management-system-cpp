#include "AdvancedSearch.h"
#include "DateUtil.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_set>

// ==== 内部辅助函数 =============================================================

namespace {

    // 将文本分割成单词（转换为小写）
    std::set<std::string> splitWords(const std::string& text) {
        std::set<std::string> tokens;
        std::string token;
        for (char ch : text) {
            if (std::isalnum(static_cast<unsigned char>(ch))) {
                token.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }
            else if (!token.empty()) {
                tokens.insert(token);
                token.clear();
            }
        }
        if (!token.empty()) {
            tokens.insert(token);
        }
        return tokens;
    }

    // 检查文本中是否包含短语（忽略大小写）
    bool containsPhrase(const std::string& text, const std::string& phrase) {
        return StringUtil::containsIgnoreCase(text, phrase);
    }

    // 计算两个字符串的编辑距离相似度
    double editDistanceSimilarity(const std::string& source, const std::string& target) {
        const std::string a = StringUtil::toLower(source);
        const std::string b = StringUtil::toLower(target);

        // 动态规划计算编辑距离
        std::vector<int> prev(b.size() + 1);
        std::vector<int> curr(b.size() + 1);
        std::iota(prev.begin(), prev.end(), 0);

        for (std::size_t i = 1; i <= a.size(); ++i) {
            curr[0] = static_cast<int>(i);
            for (std::size_t j = 1; j <= b.size(); ++j) {
                int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                curr[j] = std::min({ prev[j] + 1,
                                     curr[j - 1] + 1,
                                     prev[j - 1] + cost });
            }
            std::swap(prev, curr);
        }

        int distance = prev[b.size()];
        int maxLen = static_cast<int>(std::max(a.size(), b.size()));
        if (maxLen == 0) {
            return 1.0;
        }
        return 1.0 - static_cast<double>(distance) / static_cast<double>(maxLen);
    }

} // namespace

// ==== 构造函数 ==============================================================

AdvancedSearch::AdvancedSearch(BookService& bookService, ReaderService& readerService)
    : m_bookService(bookService),
    m_readerService(readerService) {}

// ==== 基础搜索 ==============================================================

// 关键词搜索
std::vector<Book> AdvancedSearch::searchByKeyword(const std::string& keyword) {
    updateSearchFrequency(keyword);
    return m_bookService.searchByKeyword(keyword);
}

// 短语搜索（精确匹配）
std::vector<Book> AdvancedSearch::searchByPhrase(const std::string& phrase) {
    updateSearchFrequency(phrase);
    auto books = m_bookService.searchByKeyword(phrase);

    // 筛选包含短语的书籍
    std::vector<Book> filtered;
    for (const auto& book : books) {
        if (containsPhrase(book.getTitle(), phrase) ||
            containsPhrase(book.getAuthor(), phrase) ||
            containsPhrase(book.getCategory(), phrase) ||
            containsPhrase(book.getPublisher(), phrase)) {
            filtered.push_back(book);
        }
    }
    return filtered;
}

// ==== 高级搜索 ==============================================================

// 多字段搜索
std::vector<Book> AdvancedSearch::multiFieldSearch(const SearchQuery& query) {
    std::vector<Book> results;
    auto allBooks = m_bookService.searchByKeyword("");

    // 遍历所有书籍，检查是否匹配查询条件
    for (const auto& book : allBooks) {
        bool match = true;
        if (query.keyword.has_value() &&
            !book.matchesKeyword(query.keyword.value())) {
            match = false;
        }
        if (match && query.title.has_value() &&
            !StringUtil::containsIgnoreCase(book.getTitle(), query.title.value())) {
            match = false;
        }
        if (match && query.author.has_value() &&
            !StringUtil::containsIgnoreCase(book.getAuthor(), query.author.value())) {
            match = false;
        }
        if (match && query.category.has_value() &&
            !StringUtil::containsIgnoreCase(book.getCategory(), query.category.value())) {
            match = false;
        }
        if (match && query.publisher.has_value() &&
            !StringUtil::containsIgnoreCase(book.getPublisher(), query.publisher.value())) {
            match = false;
        }
        if (match && query.year.has_value() &&
            book.getPublishYear() != query.year.value()) {
            match = false;
        }
        if (match) {
            results.push_back(book);
        }
    }
    return results;
}

// 高级过滤搜索
std::vector<Book> AdvancedSearch::advancedFilterSearch(const FilterCriteria& filters) {
    std::vector<Book> results;
    auto allBooks = m_bookService.searchByKeyword("");
    for (const auto& book : allBooks) {
        bool match = true;
        // 类别过滤
        if (filters.categories.has_value() &&
            !filters.categories->empty() &&
            filters.categories->count(book.getCategory()) == 0) {
            match = false;
        }
        // 年份范围过滤
        if (match && filters.yearRange.has_value()) {
            auto [start, end] = filters.yearRange.value();
            if (book.getPublishYear() < start || book.getPublishYear() > end) {
                match = false;
            }
        }
        // 库存范围过滤
        if (match && filters.stockRange.has_value()) {
            auto [minStock, maxStock] = filters.stockRange.value();
            if (book.getStock() < minStock || book.getStock() > maxStock) {
                match = false;
            }
        }
        // 仅显示有库存
        if (match && filters.inStockOnly.has_value()) {
            bool requireInStock = filters.inStockOnly.value();
            if (requireInStock && book.getStock() <= 0) {
                match = false;
            }
        }
        if (match) {
            results.push_back(book);
        }
    }
    return results;
}

// ==== 智能搜索 ==============================================================

// 语义搜索（基于TF-IDF）
std::vector<Book> AdvancedSearch::semanticSearch(const std::string& query) {
    updateSearchFrequency(query);
    auto keywords = tokenize(query);
    auto allBooks = m_bookService.searchByKeyword("");

    // 计算每本书的相关性得分
    std::vector<std::pair<Book, double>> scored;
    for (const auto& book : allBooks) {
        double score = 0.0;
        for (const auto& term : keywords) {
            score += computeTfIdfScore(book, term);
        }
        if (score > 0.0) {
            scored.emplace_back(book, score);
        }
    }

    // 按得分排序
    std::sort(scored.begin(), scored.end(), [](const auto& lhs, const auto& rhs) {
        if (std::abs(lhs.second - rhs.second) < 1e-6) {
            return lhs.first.getISBN() < rhs.first.getISBN();  // 得分相近按ISBN排序
        }
        return lhs.second > rhs.second;  // 按得分降序
        });

    // 提取书籍对象
    std::vector<Book> results;
    for (const auto& [book, _score] : scored) {
        results.push_back(book);
    }
    return results;
}

// 模糊搜索（基于编辑距离）
std::vector<Book> AdvancedSearch::fuzzySearch(const std::string& query, int tolerance) {
    auto books = m_bookService.searchByKeyword("");
    std::vector<Book> results;
    for (const auto& book : books) {
        // 计算查询与各字段的最大相似度
        double similarity = std::max({
            editDistanceSimilarity(book.getTitle(), query),
            editDistanceSimilarity(book.getAuthor(), query),
            editDistanceSimilarity(book.getCategory(), query),
            editDistanceSimilarity(book.getPublisher(), query)
            });
        // 容忍度转换为相似度阈值
        if (similarity >= 1.0 - tolerance * 0.1) {
            results.push_back(book);
        }
    }
    return results;
}

// 同义词搜索（简化实现）
std::vector<Book> AdvancedSearch::synonymSearch(const std::string& query) {
    // 简化：使用原始查询及其小写形式进行搜索
    std::vector<Book> result = m_bookService.searchByKeyword(query);
    if (query != StringUtil::toLower(query)) {
        auto more = m_bookService.searchByKeyword(StringUtil::toLower(query));
        result.insert(result.end(), more.begin(), more.end());
    }
    return result;
}

// ==== 搜索优化 ==============================================================

// 获取搜索建议
std::vector<std::string> AdvancedSearch::getSearchSuggestions(const std::string& prefix) {
    std::vector<std::string> suggestions;
    auto books = m_bookService.searchByTitle(prefix);
    for (const auto& book : books) {
        if (StringUtil::startsWith(book.getTitle(), prefix, true)) {
            suggestions.push_back(book.getTitle());
        }
    }
    // 去重和限制数量
    std::sort(suggestions.begin(), suggestions.end());
    suggestions.erase(std::unique(suggestions.begin(), suggestions.end()), suggestions.end());
    if (suggestions.size() > 10) {
        suggestions.resize(10);
    }
    return suggestions;
}

// 获取热门搜索
std::vector<std::string> AdvancedSearch::getPopularSearches(int limit) {
    std::vector<std::pair<std::string, int>> entries(m_searchFrequency.begin(), m_searchFrequency.end());
    std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;  // 次数相同按查询词排序
        }
        return lhs.second > rhs.second;  // 按搜索次数降序
        });

    std::vector<std::string> popular;
    for (const auto& [query, count] : entries) {
        (void)count;  // 忽略未使用的变量
        popular.push_back(query);
        if (limit > 0 && static_cast<int>(popular.size()) >= limit) {
            break;
        }
    }
    return popular;
}

// 保存搜索历史
void AdvancedSearch::saveSearchHistory(const std::string& readerId, const std::string& query) {
    if (readerId.empty() || query.empty()) {
        return;
    }
    auto& history = m_readerHistory[readerId];
    history.push_back(query);
    if (history.size() > 20) {
        history.erase(history.begin());  // 保持历史记录不超过20条
    }
    updateSearchFrequency(query);
}

// ==== 全文检索 ==============================================================

// 构建全文索引
bool AdvancedSearch::buildFullTextIndex() {
    m_invertedIndex.clear();
    auto books = m_bookService.searchByKeyword("");
    for (const auto& book : books) {
        // 从标题、作者、类别中提取词条
        auto tokens = tokenize(book.getTitle());
        auto tokensAuthor = tokenize(book.getAuthor());
        tokens.insert(tokensAuthor.begin(), tokensAuthor.end());
        auto tokensCategory = tokenize(book.getCategory());
        tokens.insert(tokensCategory.begin(), tokensCategory.end());

        // 更新倒排索引
        for (const auto& token : tokens) {
            m_invertedIndex[token].insert(book.getISBN());
        }
    }
    return true;
}

// 全文搜索
std::vector<Book> AdvancedSearch::fullTextSearch(const std::string& query) {
    updateSearchFrequency(query);
    auto tokens = tokenize(query);
    if (tokens.empty()) {
        return {};
    }

    // 取所有词条的交集
    std::set<std::string> resultISBNs;
    bool first = true;

    for (const auto& token : tokens) {
        auto it = m_invertedIndex.find(token);
        if (it == m_invertedIndex.end()) {
            continue;
        }
        if (first) {
            resultISBNs = it->second;
            first = false;
        }
        else {
            std::set<std::string> temp;
            std::set_intersection(resultISBNs.begin(), resultISBNs.end(),
                it->second.begin(), it->second.end(),
                std::inserter(temp, temp.begin()));
            resultISBNs = std::move(temp);
        }
        if (resultISBNs.empty()) {
            break;
        }
    }

    // 将ISBN转换为书籍对象
    std::vector<Book> results;
    for (const auto& isbn : resultISBNs) {
        auto books = m_bookService.searchByISBN(isbn);
        if (!books.empty()) {
            results.push_back(books.front());
        }
    }
    return results;
}

// 获取搜索相关性得分
double AdvancedSearch::getSearchRelevanceScore(const Book& book, const std::string& query) {
    auto tokens = tokenize(query);
    if (tokens.empty()) {
        return 0.0;
    }

    double score = 0.0;
    for (const auto& token : tokens) {
        score += computeTfIdfScore(book, token);
    }
    return score;
}

// ==== 私有工具 ==============================================================

// 更新搜索频率统计
void AdvancedSearch::updateSearchFrequency(const std::string& query) {
    if (query.empty()) {
        return;
    }
    m_searchFrequency[StringUtil::toLower(query)]++;
}

// 分词
std::set<std::string> AdvancedSearch::tokenize(const std::string& text) const {
    return splitWords(text);
}

// 计算TF-IDF得分
double AdvancedSearch::computeTfIdfScore(const Book& book, const std::string& term) const {
    // 词频(TF)计算
    double tf = 0.0;
    auto tokensTitle = tokenize(book.getTitle());
    auto tokensAuthor = tokenize(book.getAuthor());
    auto tokensCategory = tokenize(book.getCategory());
    if (tokensTitle.count(term) > 0) {
        tf += 2.0;  // 标题中出现的权重更高
    }
    if (tokensAuthor.count(term) > 0) {
        tf += 1.5;
    }
    if (tokensCategory.count(term) > 0) {
        tf += 1.0;
    }

    if (tf == 0.0) {
        return 0.0;
    }

    // 逆文档频率(IDF)计算
    int docCount = static_cast<int>(m_invertedIndex.size());
    if (docCount == 0) {
        return tf;
    }
    auto it = m_invertedIndex.find(term);
    int docFrequency = (it != m_invertedIndex.end()) ? static_cast<int>(it->second.size()) : 1;
    double idf = std::log(static_cast<double>(docCount) / static_cast<double>(docFrequency));
    return tf * idf;
}
