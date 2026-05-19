#include "CommunityManager.h"
#include "DateUtil.h"
#include <sstream>
#include <algorithm>
#include <unordered_set>

// ==== 构造函数 ==============================================================

// 构造函数
CommunityManager::CommunityManager(BookService& bookService,
    ReaderService& readerService,
    NotificationCenter& notificationCenter)
    : m_bookService(bookService),
    m_readerService(readerService),
    m_notificationCenter(notificationCenter) {}

// ==== 讨论线程管理 =========================================================

// 创建讨论线程
std::optional<std::string> CommunityManager::createThread(const std::string& bookIsbn,
    const std::string& title,
    const std::string& creatorId) {
    // 验证图书和读者是否存在
    if (!ensureBookExists(bookIsbn) || !ensureReaderValid(creatorId)) {
        return std::nullopt;
    }
    if (StringUtil::trim(title).empty()) {
        return std::nullopt;
    }

    // 创建线程对象
    DiscussionThread thread;
    thread.threadId = generateThreadId();
    thread.bookIsbn = bookIsbn;
    thread.title = title;
    thread.creatorId = creatorId;
    thread.createdAt = DateUtil::today();
    thread.postIds.clear();
    thread.participantIds = { creatorId };  // 创建者自动加入参与者列表

    // 保存线程
    m_threads.emplace(thread.threadId, thread);
    m_postsByThread[thread.threadId] = {};  // 初始化帖子列表

    return thread.threadId;
}

// 关闭讨论线程
bool CommunityManager::closeThread(const std::string& threadId) {
    auto it = m_threads.find(threadId);
    if (it == m_threads.end()) {
        return false;
    }

    // 删除关联帖子与评论
    auto postsIt = m_postsByThread.find(threadId);
    if (postsIt != m_postsByThread.end()) {
        for (const auto& postId : postsIt->second) {
            auto commentsIt = m_commentsByPost.find(postId);
            if (commentsIt != m_commentsByPost.end()) {
                // 删除该帖子的所有评论
                for (const auto& commentId : commentsIt->second) {
                    m_comments.erase(commentId);
                }
                m_commentsByPost.erase(commentsIt);
            }
            m_posts.erase(postId);  // 删除帖子
        }
        m_postsByThread.erase(postsIt);
    }

    m_threads.erase(it);  // 删除线程
    return true;
}

// 按图书列出线程
std::vector<DiscussionThread> CommunityManager::listThreadsByBook(const std::string& bookIsbn) const {
    std::vector<DiscussionThread> results;
    for (const auto& [threadId, thread] : m_threads) {
        if (StringUtil::equalsIgnoreCase(thread.bookIsbn, bookIsbn)) {
            results.push_back(thread);
        }
    }
    return results;
}

// 按读者列出线程
std::vector<DiscussionThread> CommunityManager::listThreadsByReader(const std::string& readerId) const {
    std::vector<DiscussionThread> results;
    for (const auto& [threadId, thread] : m_threads) {
        // 如果是创建者或参与者
        if (thread.creatorId == readerId ||
            std::find(thread.participantIds.begin(), thread.participantIds.end(), readerId) != thread.participantIds.end()) {
            results.push_back(thread);
        }
    }
    return results;
}

// 获取线程详情
std::optional<DiscussionThread> CommunityManager::getThread(const std::string& threadId) const {
    auto it = m_threads.find(threadId);
    if (it == m_threads.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ==== 帖子与评论 ===========================================================

// 添加帖子
std::optional<std::string> CommunityManager::addPost(const std::string& threadId,
    const std::string& authorId,
    const std::string& content) {
    if (!ensureReaderValid(authorId)) {
        return std::nullopt;
    }
    auto threadIt = m_threads.find(threadId);
    if (threadIt == m_threads.end()) {
        return std::nullopt;
    }
    if (StringUtil::trim(content).empty()) {
        return std::nullopt;
    }

    CommunityPost post;
    post.postId = generatePostId();
    post.threadId = threadId;
    post.authorId = authorId;
    post.content = content;
    post.createdAt = DateUtil::today();
    post.likeCount = 0;
    post.likedReaders.clear();

    // 保存帖子
    m_posts.emplace(post.postId, post);
    m_postsByThread[threadId].push_back(post.postId);
    trackParticipant(threadId, authorId);  // 跟踪参与者

    return post.postId;
}

// 点赞帖子
bool CommunityManager::likePost(const std::string& postId, const std::string& readerId) {
    if (!ensureReaderValid(readerId)) {
        return false;
    }
    auto it = m_posts.find(postId);
    if (it == m_posts.end()) {
        return false;
    }

    auto& post = it->second;
    // 检查是否已点赞
    if (std::find(post.likedReaders.begin(), post.likedReaders.end(), readerId) != post.likedReaders.end()) {
        return false;
    }
    post.likedReaders.push_back(readerId);
    ++post.likeCount;
    trackParticipant(post.threadId, readerId);  // 跟踪参与者
    return true;
}

// 取消点赞帖子
bool CommunityManager::unlikePost(const std::string& postId, const std::string& readerId) {
    auto it = m_posts.find(postId);
    if (it == m_posts.end()) {
        return false;
    }
    auto& post = it->second;
    auto pos = std::find(post.likedReaders.begin(), post.likedReaders.end(), readerId);
    if (pos == post.likedReaders.end()) {
        return false;
    }
    post.likedReaders.erase(pos);
    post.likeCount = std::max(0, post.likeCount - 1);
    return true;
}

// 添加评论
std::optional<std::string> CommunityManager::addComment(const std::string& postId,
    const std::string& authorId,
    const std::string& content) {
    if (!ensureReaderValid(authorId)) {
        return std::nullopt;
    }
    auto postIt = m_posts.find(postId);
    if (postIt == m_posts.end()) {
        return std::nullopt;
    }
    if (StringUtil::trim(content).empty()) {
        return std::nullopt;
    }

    CommunityComment comment;
    comment.commentId = generateCommentId();
    comment.postId = postId;
    comment.authorId = authorId;
    comment.content = content;
    comment.createdAt = DateUtil::today();

    // 保存评论
    m_comments.emplace(comment.commentId, comment);
    m_commentsByPost[postId].push_back(comment.commentId);
    trackParticipant(postIt->second.threadId, authorId);  // 跟踪参与者
    return comment.commentId;
}

// 列出线程的所有帖子
std::vector<CommunityPost> CommunityManager::listPosts(const std::string& threadId) const {
    std::vector<CommunityPost> results;
    auto it = m_postsByThread.find(threadId);
    if (it == m_postsByThread.end()) {
        return results;
    }
    for (const auto& postId : it->second) {
        auto postIt = m_posts.find(postId);
        if (postIt != m_posts.end()) {
            results.push_back(postIt->second);
        }
    }
    return results;
}

// 列出帖子的所有评论
std::vector<CommunityComment> CommunityManager::listComments(const std::string& postId) const {
    std::vector<CommunityComment> results;
    auto it = m_commentsByPost.find(postId);
    if (it == m_commentsByPost.end()) {
        return results;
    }
    for (const auto& commentId : it->second) {
        auto cIt = m_comments.find(commentId);
        if (cIt != m_comments.end()) {
            results.push_back(cIt->second);
        }
    }
    return results;
}

// ==== 举报与审核 ===========================================================

// 举报内容
std::optional<std::string> CommunityManager::reportContent(const std::string& targetId,
    const std::string& reporterId,
    const std::string& reason) {
    if (!ensureReaderValid(reporterId) || StringUtil::trim(reason).empty()) {
        return std::nullopt;
    }
    // 检查目标是否存在
    bool targetExists = m_posts.contains(targetId) || m_comments.contains(targetId) || m_threads.contains(targetId);
    if (!targetExists) {
        return std::nullopt;
    }

    CommunityReport report;
    report.reportId = generateReportId();
    report.targetId = targetId;
    report.reporterId = reporterId;
    report.reason = reason;
    report.createdAt = DateUtil::today();
    report.resolved = false;

    m_reports.emplace(report.reportId, report);
    return report.reportId;
}

// 处理举报
bool CommunityManager::resolveReport(const std::string& reportId, bool actionTaken) {
    auto it = m_reports.find(reportId);
    if (it == m_reports.end()) {
        return false;
    }
    it->second.resolved = true;  // 标记为已处理

    // 如果采取行动，删除被举报内容
    if (actionTaken) {
        const std::string& targetId = it->second.targetId;
        if (m_comments.erase(targetId) > 0) {
            // 从帖子的评论列表中删除
            for (auto& [postId, commentIds] : m_commentsByPost) {
                commentIds.erase(std::remove(commentIds.begin(), commentIds.end(), targetId), commentIds.end());
            }
        }
        else if (m_posts.erase(targetId) > 0) {
            // 从线程的帖子列表中删除
            for (auto& [threadId, postIds] : m_postsByThread) {
                postIds.erase(std::remove(postIds.begin(), postIds.end(), targetId), postIds.end());
            }
        }
        else if (m_threads.erase(targetId) > 0) {
            closeThread(targetId);  // 关闭线程
        }
    }
    return true;
}

// ==== 统计与推荐 ===========================================================

// 获取社区统计信息
CommunityStats CommunityManager::getCommunityStats() const {
    CommunityStats stats{};
    stats.threadCount = static_cast<int>(m_threads.size());
    stats.postCount = static_cast<int>(m_posts.size());
    stats.commentCount = static_cast<int>(m_comments.size());

    // 统计活跃读者（去重）
    std::unordered_set<std::string> readers;
    for (const auto& [threadId, thread] : m_threads) {
        (void)threadId;  // 忽略未使用的变量
        readers.insert(thread.participantIds.begin(), thread.participantIds.end());
    }
    stats.activeReaderCount = static_cast<int>(readers.size());
    return stats;
}

// 获取热门线程
std::vector<DiscussionThread> CommunityManager::getTrendingThreads(int limit) const {
    std::vector<std::pair<std::string, int>> ranking;
    for (const auto& [threadId, thread] : m_threads) {
        int score = 0;
        auto posts = listPosts(threadId);
        score += static_cast<int>(posts.size()) * 2;  // 帖子数量权重
        for (const auto& post : posts) {
            score += post.likeCount;  // 点赞数量
        }
        ranking.emplace_back(threadId, score);
    }

    // 按得分排序
    std::sort(ranking.begin(), ranking.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;  // 得分相同按ID排序
        }
        return lhs.second > rhs.second;  // 按得分降序
        });

    // 转换为线程对象
    std::vector<DiscussionThread> result;
    for (const auto& [threadId, _score] : ranking) {
        auto it = m_threads.find(threadId);
        if (it != m_threads.end()) {
            result.push_back(it->second);
        }
        if (limit > 0 && static_cast<int>(result.size()) >= limit) {
            break;
        }
    }
    return result;
}

// 获取热门帖子
std::vector<CommunityPost> CommunityManager::getPopularPosts(int limit) const {
    std::vector<CommunityPost> posts;
    posts.reserve(m_posts.size());
    for (const auto& [postId, post] : m_posts) {
        (void)postId;  // 忽略未使用的变量
        posts.push_back(post);
    }

    // 按点赞数排序
    std::sort(posts.begin(), posts.end(), [](const CommunityPost& lhs, const CommunityPost& rhs) {
        if (lhs.likeCount == rhs.likeCount) {
            return lhs.postId < rhs.postId;  // 点赞数相同按ID排序
        }
        return lhs.likeCount > rhs.likeCount;  // 按点赞数降序
        });

    // 限制返回数量
    if (limit > 0 && static_cast<std::size_t>(limit) < posts.size()) {
        posts.resize(static_cast<std::size_t>(limit));
    }
    return posts;
}

// ==== 邀请与通知 ===========================================================

// 邀请读者加入线程
bool CommunityManager::inviteReaderToThread(const std::string& readerId, const std::string& threadId) {
    if (!ensureReaderValid(readerId)) {
        return false;
    }
    auto it = m_threads.find(threadId);
    if (it == m_threads.end()) {
        return false;
    }
    trackParticipant(threadId, readerId);  // 将读者加入参与者列表

    std::ostringstream oss;
    oss << "您被邀请加入讨论串《" << it->second.title << "》。";
    return m_notificationCenter.sendCustomNotification(readerId, oss.str());  // 发送通知
}

// 通知线程更新
bool CommunityManager::notifyThreadUpdate(const std::string& threadId, const std::string& message) {
    auto it = m_threads.find(threadId);
    if (it == m_threads.end() || StringUtil::trim(message).empty()) {
        return false;
    }

    // 通知所有参与者
    bool sent = false;
    for (const auto& readerId : it->second.participantIds) {
        sent = m_notificationCenter.sendCustomNotification(readerId, message) || sent;
    }
    return sent;
}

// ==== 内部工具 =============================================================

// 生成线程ID
std::string CommunityManager::generateThreadId() {
    ++m_threadSequence;
    return "THR-" + std::to_string(m_threadSequence);
}

// 生成帖子ID
std::string CommunityManager::generatePostId() {
    ++m_postSequence;
    return "PST-" + std::to_string(m_postSequence);
}

// 生成评论ID
std::string CommunityManager::generateCommentId() {
    ++m_commentSequence;
    return "CMT-" + std::to_string(m_commentSequence);
}

// 生成举报ID
std::string CommunityManager::generateReportId() {
    ++m_reportSequence;
    return "RPT-" + std::to_string(m_reportSequence);
}

// 验证读者是否有效
bool CommunityManager::ensureReaderValid(const std::string& readerId) const {
    const Reader* reader = m_readerService.getReaderById(readerId);
    return reader != nullptr && reader->isActive() && !reader->isSuspended();
}

// 验证图书是否存在
bool CommunityManager::ensureBookExists(const std::string& isbn) const {
    auto books = m_bookService.searchByISBN(isbn);
    return !books.empty();
}

// 跟踪参与者（将读者添加到线程的参与者列表）
void CommunityManager::trackParticipant(const std::string& threadId, const std::string& readerId) {
    auto it = m_threads.find(threadId);
    if (it == m_threads.end()) {
        return;
    }
    auto& participants = it->second.participantIds;
    if (std::find(participants.begin(), participants.end(), readerId) == participants.end()) {
        participants.push_back(readerId);
    }
}
