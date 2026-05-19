#ifndef COMMUNITY_MANAGER_H
#define COMMUNITY_MANAGER_H

#include "BookService.h"
#include "NotificationCenter.h"
#include "ReaderService.h"
#include "StringUtil.h"

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @struct CommunityPost
 * @brief 社区帖子实体，记录帖子内容、作者与互动数据。
 */
struct CommunityPost {
    std::string postId;
    std::string threadId;
    std::string authorId;
    std::string content;
    Date createdAt;
    int likeCount{ 0 };
    std::vector<std::string> likedReaders;
};

/**
 * @struct CommunityComment
 * @brief 社区评论实体。
 */
struct CommunityComment {
    std::string commentId;
    std::string postId;
    std::string authorId;
    std::string content;
    Date createdAt;
};

/**
 * @struct DiscussionThread
 * @brief 讨论帖实体，关联图书与帖子列表。
 */
struct DiscussionThread {
    std::string threadId;
    std::string bookIsbn;
    std::string title;
    std::string creatorId;
    Date createdAt;
    std::vector<std::string> postIds;
    std::vector<std::string> participantIds;
};

/**
 * @struct CommunityReport
 * @brief 社区举报记录。
 */
struct CommunityReport {
    std::string reportId;
    std::string targetId;
    std::string reporterId;
    std::string reason;
    Date createdAt;
    bool resolved{ false };
};

/**
 * @struct CommunityStats
 * @brief 社区统计信息。
 */
struct CommunityStats {
    int threadCount{ 0 };
    int postCount{ 0 };
    int commentCount{ 0 };
    int activeReaderCount{ 0 };
};

/**
 * @class CommunityManager
 * @brief 高级功能层社区管理模块，负责读者互动、讨论与举报处理。
 */
class CommunityManager {
public:
    CommunityManager(BookService& bookService,
        ReaderService& readerService,
        NotificationCenter& notificationCenter);

    // === 讨论线程管理 ===
    std::optional<std::string> createThread(const std::string& bookIsbn,
        const std::string& title,
        const std::string& creatorId);
    bool closeThread(const std::string& threadId);
    std::vector<DiscussionThread> listThreadsByBook(const std::string& bookIsbn) const;
    std::vector<DiscussionThread> listThreadsByReader(const std::string& readerId) const;
    std::optional<DiscussionThread> getThread(const std::string& threadId) const;

    // === 帖子与评论 ===
    std::optional<std::string> addPost(const std::string& threadId,
        const std::string& authorId,
        const std::string& content);
    bool likePost(const std::string& postId, const std::string& readerId);
    bool unlikePost(const std::string& postId, const std::string& readerId);
    std::optional<std::string> addComment(const std::string& postId,
        const std::string& authorId,
        const std::string& content);
    std::vector<CommunityPost> listPosts(const std::string& threadId) const;
    std::vector<CommunityComment> listComments(const std::string& postId) const;

    // === 举报与审核 ===
    std::optional<std::string> reportContent(const std::string& targetId,
        const std::string& reporterId,
        const std::string& reason);
    bool resolveReport(const std::string& reportId, bool actionTaken);

    // === 统计与推荐 ===
    CommunityStats getCommunityStats() const;
    std::vector<DiscussionThread> getTrendingThreads(int limit = 5) const;
    std::vector<CommunityPost> getPopularPosts(int limit = 5) const;

    // === 邀请与通知 ===
    bool inviteReaderToThread(const std::string& readerId, const std::string& threadId);
    bool notifyThreadUpdate(const std::string& threadId, const std::string& message);

private:
    BookService& m_bookService;
    ReaderService& m_readerService;
    NotificationCenter& m_notificationCenter;

    long long m_threadSequence{ 0 };
    long long m_postSequence{ 0 };
    long long m_commentSequence{ 0 };
    long long m_reportSequence{ 0 };

    std::unordered_map<std::string, DiscussionThread> m_threads;
    std::unordered_map<std::string, CommunityPost> m_posts;
    std::unordered_map<std::string, CommunityComment> m_comments;
    std::unordered_map<std::string, CommunityReport> m_reports;

    std::unordered_map<std::string, std::vector<std::string>> m_postsByThread;
    std::unordered_map<std::string, std::vector<std::string>> m_commentsByPost;

    // === 内部工具 ===
    std::string generateThreadId();
    std::string generatePostId();
    std::string generateCommentId();
    std::string generateReportId();

    bool ensureReaderValid(const std::string& readerId) const;
    bool ensureBookExists(const std::string& isbn) const;
    void trackParticipant(const std::string& threadId, const std::string& readerId);
};

#endif // COMMUNITY_MANAGER_H
