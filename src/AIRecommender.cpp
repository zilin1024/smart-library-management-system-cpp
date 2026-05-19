#include "AIRecommender.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace {

    // 计算两本书的相似度得分
    double similarityScore(const Book& a, const Book& b) {
        double score = 0.0;
        // 类别相同加2分
        if (StringUtil::equalsIgnoreCase(a.getCategory(), b.getCategory())) {
            score += 2.0;
        }
        // 作者相同加3分
        if (StringUtil::equalsIgnoreCase(a.getAuthor(), b.getAuthor())) {
            score += 3.0;
        }
        // 出版社相同加1分
        if (StringUtil::equalsIgnoreCase(a.getPublisher(), b.getPublisher())) {
            score += 1.0;
        }

        // 处理标签匹配
        std::unordered_set<std::string> tagsA;
        for (const auto& tag : a.getTags()) {
            tagsA.insert(StringUtil::toLower(tag));
        }

        double tagMatches = 0.0;
        for (const auto& tag : b.getTags()) {
            if (tagsA.count(StringUtil::toLower(tag)) > 0) {
                tagMatches += 0.5;  // 每个匹配标签加0.5分
            }
        }
        score += tagMatches;

        // 根据出版年份差异加分
        int yearDiff = std::abs(a.getPublishYear() - b.getPublishYear());
        if (yearDiff <= 1) {
            score += 1.0;
        }
        else if (yearDiff <= 3) {
            score += 0.5;
        }

        return score;
    }

    // 合并两个书单，去重，可限制总数
    std::vector<Book> mergeUniqueBooks(const std::vector<Book>& base,
        const std::vector<Book>& addition,
        size_t limit = 0) {
        std::vector<Book> result = base;
        std::unordered_set<std::string> seen;
        for (const auto& book : base) {
            seen.insert(book.getISBN());
        }

        for (const auto& book : addition) {
            if (seen.insert(book.getISBN()).second) {  // 如果ISBN未出现过
                result.push_back(book);
                if (limit > 0 && result.size() >= limit) {
                    break;  // 达到限制数量则停止
                }
            }
        }
        return result;
    }

} // namespace

// 构造函数
AIRecommender::AIRecommender(BookService& bookService,
    BorrowService& borrowService,
    ReaderService& readerService,
    SearchService& searchService)
    : m_bookService(bookService),
    m_borrowService(borrowService),
    m_readerService(readerService),
    m_searchService(searchService) {}

// 为读者生成推荐书单
std::vector<Book> AIRecommender::recommendForReader(const std::string& readerId, int limit) {
    std::vector<Book> recommendations;

    // 基于历史记录的推荐
    auto historyBooks = recommendBasedOnHistory(readerId);
    recommendations = mergeUniqueBooks(recommendations, historyBooks, static_cast<size_t>(limit));

    // 如果数量不足，添加协同过滤推荐
    if (static_cast<int>(recommendations.size()) < limit) {
        auto collaborative = collaborativeFiltering(readerId);
        recommendations = mergeUniqueBooks(recommendations, collaborative, static_cast<size_t>(limit));
    }

    // 如果数量仍不足，添加热门书籍
    if (static_cast<int>(recommendations.size()) < limit) {
        auto popular = m_searchService.getBooksBorrowedTogether("", limit);
        recommendations = mergeUniqueBooks(recommendations, popular, static_cast<size_t>(limit));
    }

    // 限制返回数量
    return limit > 0 ? std::vector<Book>(recommendations.begin(),
        recommendations.begin() + std::min(static_cast<size_t>(limit), recommendations.size()))
        : recommendations;
}

// 基于读者历史记录的推荐
std::vector<Book> AIRecommender::recommendBasedOnHistory(const std::string& readerId) {
    auto historyBooks = getReaderHistoryBooks(readerId);
    if (historyBooks.empty()) {
        return {};
    }

    // 计算每本书的推荐得分
    std::map<std::string, double> scores;
    for (const auto& historyBook : historyBooks) {
        auto similar = recommendSimilarBooks(historyBook.getISBN());
        for (const auto& candidate : similar) {
            if (candidate.getISBN() == historyBook.getISBN()) {
                continue;  // 跳过历史中的书
            }
            scores[candidate.getISBN()] += similarityScore(historyBook, candidate);
        }
    }

    // 按得分排序
    std::vector<std::pair<std::string, double>> ranking(scores.begin(), scores.end());
    std::sort(ranking.begin(), ranking.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;  // 得分相同按ISBN排序
        }
        return lhs.second > rhs.second;  // 按得分降序
        });

    // 转换为书对象
    std::vector<Book> results;
    for (const auto& [isbn, _score] : ranking) {
        auto books = m_bookService.searchByISBN(isbn);
        if (!books.empty()) {
            results.push_back(books.front());
        }
    }
    return results;
}

// 推荐与指定书籍相似的书籍
std::vector<Book> AIRecommender::recommendSimilarBooks(const std::string& isbn) {
    auto baseBooks = m_bookService.searchByISBN(isbn);
    if (baseBooks.empty()) {
        return {};
    }
    const Book& target = baseBooks.front();

    // 获取候选书籍
    auto candidates = m_bookService.searchByCategory(target.getCategory());
    candidates = mergeUniqueBooks(candidates, m_bookService.searchByAuthor(target.getAuthor()));

    // 计算每本书的相似度得分
    std::vector<std::pair<Book, double>> scored;
    for (const auto& candidate : candidates) {
        if (candidate.getISBN() == target.getISBN()) {
            continue;  // 跳过目标书籍本身
        }
        double score = similarityScore(target, candidate);
        if (score > 0.0) {  // 只保留有相似度的
            scored.emplace_back(candidate, score);
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
    for (const auto& [book, score] : scored) {
        (void)score;  // 忽略未使用的变量
        results.push_back(book);
    }
    return results;
}

// 获取最常借阅的书籍
std::vector<Book> AIRecommender::getTopBorrowedBooks(int days, int limit) {
    auto popularMap = m_borrowService.getPopularBooks(limit > 0 ? limit * 5 : 50);

    // 按借阅次数排序
    std::vector<std::pair<std::string, int>> ranking(popularMap.begin(), popularMap.end());
    std::sort(ranking.begin(), ranking.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;  // 次数相同按ISBN排序
        }
        return lhs.second > rhs.second;  // 按次数降序
        });

    // 转换为书对象
    std::vector<Book> result;
    for (const auto& [isbn, _count] : ranking) {
        auto books = m_bookService.searchByISBN(isbn);
        if (!books.empty()) {
            result.push_back(books.front());
        }
        if (limit > 0 && static_cast<int>(result.size()) >= limit) {
            break;  // 达到限制数量则停止
        }
    }
    return result;
}

// 获取评分最高的书籍
std::vector<Book> AIRecommender::getTopRatedBooks(int limit) {
    auto allBooks = m_bookService.searchByKeyword("");
    // 按评分排序，评分相同按评分人数排序
    std::sort(allBooks.begin(), allBooks.end(), [](const Book& lhs, const Book& rhs) {
        if (std::abs(lhs.getAverageRating() - rhs.getAverageRating()) < 1e-6) {
            return lhs.getRatingCount() > rhs.getRatingCount();  // 评分相近按评分人数降序
        }
        return lhs.getAverageRating() > rhs.getAverageRating();  // 按评分降序
        });

    // 限制返回数量
    if (limit > 0 && static_cast<size_t>(limit) < allBooks.size()) {
        allBooks.resize(static_cast<size_t>(limit));
    }
    return allBooks;
}

// 获取新到书籍
std::vector<Book> AIRecommender::getNewArrivals(int days, int limit) {
    Date threshold = DateUtil::addDays(DateUtil::today(), -days);  // 计算阈值日期
    auto allBooks = m_bookService.searchByKeyword("");

    // 筛选最近添加的书籍
    std::vector<Book> recent;
    for (const auto& book : allBooks) {
        if (book.getAddedDate() >= threshold) {
            recent.push_back(book);
        }
    }

    // 按添加日期排序
    std::sort(recent.begin(), recent.end(), [](const Book& lhs, const Book& rhs) {
        return lhs.getAddedDate() > rhs.getAddedDate();  // 按日期降序
        });

    // 限制返回数量
    if (limit > 0 && static_cast<size_t>(limit) < recent.size()) {
        recent.resize(static_cast<size_t>(limit));
    }
    return recent;
}

// 协同过滤推荐
std::vector<Book> AIRecommender::collaborativeFiltering(const std::string& readerId) {
    auto baseHistory = getReaderHistoryBooks(readerId);
    if (baseHistory.empty()) {
        return {};
    }

    // 获取当前读者的ISBN集合
    std::unordered_set<std::string> baseIsbns;
    for (const auto& book : baseHistory) {
        baseIsbns.insert(StringUtil::toLower(book.getISBN()));
    }

    std::unordered_map<std::string, int> similarityCounter;  // ISBN到相似度得分的映射
    auto readers = m_readerService.getReadersByBorrowCount(1);  // 获取有借阅记录的读者

    // 遍历其他读者
    for (const auto& reader : readers) {
        if (reader.getReaderId() == readerId) {
            continue;  // 跳过自己
        }

        // 获取该读者的ISBN集合
        std::unordered_set<std::string> readerIsbns;
        for (const auto& isbn : reader.getBorrowHistory()) {
            readerIsbns.insert(StringUtil::toLower(isbn));
        }

        // 计算重叠度
        int overlap = 0;
        for (const auto& isbn : baseIsbns) {
            if (readerIsbns.count(isbn) > 0) {
                ++overlap;
            }
        }

        if (overlap == 0) {
            continue;  // 没有重叠则跳过
        }

        // 将该读者有而当前读者没有的书籍加入推荐，权重为重叠度
        for (const auto& isbn : readerIsbns) {
            if (baseIsbns.count(isbn) == 0) {
                similarityCounter[isbn] += overlap;
            }
        }
    }

    // 按权重排序
    std::vector<std::pair<std::string, int>> ranking(similarityCounter.begin(), similarityCounter.end());
    std::sort(ranking.begin(), ranking.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;  // 权重相同按ISBN排序
        }
        return lhs.second > rhs.second;  // 按权重降序
        });

    // 转换为书对象
    std::vector<Book> recommendations;
    for (const auto& [isbn, score] : ranking) {
        (void)score;  // 忽略未使用的变量
        auto books = m_bookService.searchByISBN(isbn);
        if (!books.empty()) {
            recommendations.push_back(books.front());
        }
    }
    return recommendations;
}

// 获取其他读者也借阅的书籍
std::vector<Book> AIRecommender::getReadersAlsoBorrowed(const std::string& isbn) {
    return m_searchService.getBooksBorrowedTogether(isbn);
}

// 训练推荐模型（占位实现）
void AIRecommender::trainRecommendationModel() {
    // 占位实现
}

// 评估推荐准确率
double AIRecommender::evaluateRecommendationAccuracy() {
    // 占位实现，返回固定值
    return 0.75;
}

// 更新读者偏好
void AIRecommender::updateReaderPreferences(const std::string& readerId) {
    (void)readerId; 
}

// 获取读者的历史借阅书籍
std::vector<Book> AIRecommender::getReaderHistoryBooks(const std::string& readerId) const {
    std::vector<Book> historyBooks;
    const Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        return historyBooks;  // 读者不存在
    }

    // 将ISBN转换为书对象
    for (const auto& isbn : reader->getBorrowHistory()) {
        auto books = m_bookService.searchByISBN(isbn);
        if (!books.empty()) {
            historyBooks.push_back(books.front());
        }
    }
    return historyBooks;
}

// 评分并选择推荐
std::vector<Book> AIRecommender::scoreAndSelectRecommendations(const std::vector<Book>& candidates,
    const std::string& readerId,
    int limit) const {
    (void)candidates;  
    (void)readerId;
    (void)limit;
    return {};
}
