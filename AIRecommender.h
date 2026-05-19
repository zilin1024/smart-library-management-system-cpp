#ifndef AI_RECOMMENDER_H
#define AI_RECOMMENDER_H

#include "BookService.h"
#include "BorrowService.h"
#include "ReaderService.h"
#include "SearchService.h"

#include <string>
#include <vector>

/**
 * @class AIRecommender
 * @brief 高级功能层智能推荐系统，基于历史借阅、类别偏好与热门趋势生成推荐结果。
 *
 * 目前实现采用启发式算法，结合读者历史记录、热门借阅榜与相似图书信息。
 * 后续可接入更复杂的机器学习模型。
 */
class AIRecommender {
public:
    AIRecommender(BookService& bookService,
        BorrowService& borrowService,
        ReaderService& readerService,
        SearchService& searchService);

    // === 个性化推荐 ===
    std::vector<Book> recommendForReader(const std::string& readerId, int limit = 10);
    std::vector<Book> recommendBasedOnHistory(const std::string& readerId);
    std::vector<Book> recommendSimilarBooks(const std::string& isbn);

    // === 热门推荐 ===
    std::vector<Book> getTopBorrowedBooks(int days = 30, int limit = 10);
    std::vector<Book> getTopRatedBooks(int limit = 10);
    std::vector<Book> getNewArrivals(int days = 7, int limit = 10);

    // === 协同过滤（简化实现） ===
    std::vector<Book> collaborativeFiltering(const std::string& readerId);
    std::vector<Book> getReadersAlsoBorrowed(const std::string& isbn);

    // === 推荐算法管理（占位实现） ===
    void trainRecommendationModel();
    double evaluateRecommendationAccuracy();
    void updateReaderPreferences(const std::string& readerId);

private:
    BookService& m_bookService;
    BorrowService& m_borrowService;
    ReaderService& m_readerService;
    SearchService& m_searchService;

    std::vector<Book> getReaderHistoryBooks(const std::string& readerId) const;
    std::vector<Book> scoreAndSelectRecommendations(const std::vector<Book>& candidates,
        const std::string& readerId,
        int limit) const;
};

#endif // AI_RECOMMENDER_H
