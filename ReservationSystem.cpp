#include "ReservationSystem.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <algorithm>
#include <map>
#include <sstream>

namespace {

    // 辅助函数：为预约状态分配权重，用于排序
    // 顺序：生效中(0) < 已满足(1) < 已过期(2) < 已取消(3) < 其他(4)
    int statusWeight(ReservationStatus status) {
        switch (status) {
        case ReservationStatus::Active: return 0;
        case ReservationStatus::Fulfilled: return 1;
        case ReservationStatus::Expired: return 2;
        case ReservationStatus::Cancelled: return 3;
        default: return 4;
        }
    }

    // 辅助函数：对预约队列进行排序
    // 规则：状态权重 -> 优先级(高到低) -> 申请时间(早到晚) -> ID
    void sortReservations(std::vector<ReservationRecord>& queue) {
        std::sort(queue.begin(), queue.end(), [](const ReservationRecord& lhs, const ReservationRecord& rhs) {
            int lw = statusWeight(lhs.status);
            int rw = statusWeight(rhs.status);
            if (lw != rw) {
                return lw < rw;
            }
            if (lhs.status == ReservationStatus::Active && rhs.status == ReservationStatus::Active) {
                if (lhs.priority != rhs.priority) {
                    return static_cast<int>(lhs.priority) > static_cast<int>(rhs.priority);
                }
            }
            if (lhs.requestDate != rhs.requestDate) {
                return lhs.requestDate < rhs.requestDate;
            }
            return lhs.reservationId < rhs.reservationId;
            });
    }

} // namespace

// 构造函数：初始化依赖服务及默认保留天数
ReservationSystem::ReservationSystem(BookService& bookService,
    ReaderService& readerService,
    BorrowService& borrowService,
    int defaultHoldDays)
    : m_bookService(bookService),
    m_readerService(readerService),
    m_borrowService(borrowService),
    m_defaultHoldDays(defaultHoldDays),
    m_sequence(0) {}

// 创建预约
ReservationResult ReservationSystem::reserveBook(const std::string& readerId, const std::string& isbn) {
    ReservationResult result;
    std::string error;

    // 1. 检查预约前置条件（读者状态、图书是否存在、是否可以直接借阅等）
    if (!ensureReservationPrerequisites(readerId, isbn, error)) {
        result.success = false;
        result.message = error;
        return result;
    }

    // 2. 创建预约记录
    Date today = DateUtil::today();
    ReservationRecord record{
        generateReservationId(),
        readerId,
        isbn,
        today,
        DateUtil::addDays(today, m_defaultHoldDays), // 默认保留期限
        PriorityLevel::Normal,
        ReservationStatus::Active
    };

    // 3. 加入队列并重排序
    auto& queue = m_reservationsByISBN[isbn];
    queue.push_back(record);
    sortReservations(queue);
    rebuildIndex(isbn); // 重建索引以加速查找

    // 4. 计算当前排队位置
    int position = -1;
    int activeIndex = 0;
    for (const auto& entry : queue) {
        if (entry.status != ReservationStatus::Active) {
            continue;
        }
        if (entry.reservationId == record.reservationId) {
            position = activeIndex;
            break;
        }
        ++activeIndex;
    }

    result.success = true;
    result.message = "预约成功。";
    result.reservationId = record.reservationId;
    result.positionInQueue = position;
    return result;
}

// 取消预约
bool ReservationSystem::cancelReservation(const std::string& readerId, const std::string& isbn) {
    auto it = m_reservationsByISBN.find(isbn);
    if (it == m_reservationsByISBN.end()) {
        return false;
    }

    auto& queue = it->second;
    bool changed = false;
    for (auto& record : queue) {
        // 找到该读者处于激活状态的预约
        if (record.readerId == readerId && record.status == ReservationStatus::Active) {
            record.status = ReservationStatus::Cancelled;
            record.pickupDeadline = DateUtil::today();
            changed = true;
        }
    }
    if (changed) {
        sortReservations(queue);
        rebuildIndex(isbn);
    }
    return changed;
}

// 修改预约信息（如取书日期、优先级）
bool ReservationSystem::modifyReservation(const std::string& reservationId, const ReservationUpdate& update) {
    auto loc = locateReservation(reservationId);
    if (!loc.has_value()) {
        return false;
    }
    const auto& [isbn, index] = loc.value();
    auto& queue = m_reservationsByISBN[isbn];

    ReservationRecord& record = queue[index];
    if (record.status != ReservationStatus::Active) {
        return false; // 只能修改激活状态的预约
    }

    if (update.newPickupDate.has_value()) {
        record.pickupDeadline = update.newPickupDate.value();
    }
    if (update.newPriority.has_value()) {
        record.priority = update.newPriority.value();
    }

    sortReservations(queue);
    rebuildIndex(isbn);
    return true;
}

// 获取某本书的预约队列（仅返回读者ID）
std::vector<std::string> ReservationSystem::getReservationQueue(const std::string& isbn) const {
    std::vector<std::string> queueOrder;
    auto it = m_reservationsByISBN.find(isbn);
    if (it == m_reservationsByISBN.end()) {
        return queueOrder;
    }
    const auto& queue = it->second;
    for (const auto& record : queue) {
        if (record.status == ReservationStatus::Active) {
            queueOrder.push_back(record.readerId);
        }
    }
    return queueOrder;
}

// 获取某读者在某本书队列中的位置（0开始）
int ReservationSystem::getQueuePosition(const std::string& readerId, const std::string& isbn) const {
    auto it = m_reservationsByISBN.find(isbn);
    if (it == m_reservationsByISBN.end()) {
        return -1;
    }
    const auto& queue = it->second;
    int position = 0;
    for (const auto& record : queue) {
        if (record.status != ReservationStatus::Active) {
            continue;
        }
        if (record.readerId == readerId) {
            return position;
        }
        ++position;
    }
    return -1;
}

// 手动调整预约在队列中的位置（插队）
bool ReservationSystem::bumpInQueue(const std::string& reservationId, int newPosition) {
    if (newPosition < 0) {
        return false;
    }
    auto loc = locateReservation(reservationId);
    if (!loc.has_value()) {
        return false;
    }
    auto [isbn, idx] = loc.value();
    auto& queue = m_reservationsByISBN[isbn];
    if (idx >= queue.size()) {
        return false;
    }
    if (queue[idx].status != ReservationStatus::Active) {
        return false;
    }

    // 提取记录并暂时移除
    ReservationRecord record = queue[idx];
    queue.erase(queue.begin() + static_cast<std::ptrdiff_t>(idx));

    // 计算所有 Active 状态记录的索引范围
    std::vector<std::size_t> activeIndices;
    for (std::size_t i = 0; i < queue.size(); ++i) {
        if (queue[i].status == ReservationStatus::Active) {
            activeIndices.push_back(i);
        }
    }

    if (newPosition > static_cast<int>(activeIndices.size())) {
        newPosition = static_cast<int>(activeIndices.size());
    }

    // 确定插入位置
    std::size_t insertPos;
    if (newPosition == static_cast<int>(activeIndices.size())) {
        insertPos = queue.size();
    }
    else {
        insertPos = activeIndices[static_cast<std::size_t>(newPosition)];
    }

    queue.insert(queue.begin() + static_cast<std::ptrdiff_t>(insertPos), record);
    rebuildIndex(isbn);
    return true;
}

// 处理可用的预约（分配库存给排在前面的预约者）
void ReservationSystem::processAvailableReservations() {
    Date today = DateUtil::today();
    for (auto& [isbn, queue] : m_reservationsByISBN) {
        auto books = m_bookService.searchByISBN(isbn);
        if (books.empty()) {
            continue;
        }
        int stock = books.front().getStock();
        if (stock <= 0) {
            continue;
        }

        sortReservations(queue);
        // 遍历队列，将库存分配给 Active 状态的预约
        for (auto& record : queue) {
            if (stock <= 0) {
                break;
            }
            if (record.status != ReservationStatus::Active) {
                continue;
            }
            record.status = ReservationStatus::Fulfilled; // 标记为已满足
            record.pickupDeadline = DateUtil::addDays(today, m_defaultHoldDays); // 设置取书截止日期
            --stock; // 扣减临时库存（注意：此处并未真正扣减 BookService 的库存，需配合业务逻辑）
        }
        rebuildIndex(isbn);
    }
}

// 清理过期预约
void ReservationSystem::expireOldReservations() {
    Date today = DateUtil::today();
    for (auto& [isbn, queue] : m_reservationsByISBN) {
        bool changed = false;
        for (auto& record : queue) {
            // 检查 Active 或 Fulfilled 状态且超过截止日期的记录
            if ((record.status == ReservationStatus::Active || record.status == ReservationStatus::Fulfilled) &&
                record.pickupDeadline < today) {
                record.status = ReservationStatus::Expired;
                changed = true;
            }
        }
        if (changed) {
            sortReservations(queue);
            rebuildIndex(isbn);
        }
    }
}

// 通知下一位（简单封装处理逻辑）
void ReservationSystem::notifyNextInQueue(const std::string& isbn) {
    (void)isbn;
    processAvailableReservations();
}

// 获取预约统计信息
ReservationStats ReservationSystem::getReservationStatistics() const {
    ReservationStats stats{};
    for (const auto& [isbn, queue] : m_reservationsByISBN) {
        (void)isbn;
        for (const auto& record : queue) {
            ++stats.totalReservations;
            switch (record.status) {
            case ReservationStatus::Active: ++stats.activeReservations; break;
            case ReservationStatus::Fulfilled: ++stats.fulfilledReservations; break;
            case ReservationStatus::Expired: ++stats.expiredReservations; break;
            default: break;
            }
        }
    }
    return stats;
}

// 获取预约最多的图书
std::map<std::string, int> ReservationSystem::getMostReservedBooks(int limit) const {
    std::map<std::string, int> counts;
    for (const auto& [isbn, queue] : m_reservationsByISBN) {
        int activeCount = 0;
        for (const auto& record : queue) {
            if (record.status == ReservationStatus::Active) {
                ++activeCount;
            }
        }
        counts[isbn] = activeCount;
    }

    // 转换为 vector 进行排序
    std::vector<std::pair<std::string, int>> ranking(counts.begin(), counts.end());
    std::sort(ranking.begin(), ranking.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second == rhs.second) {
            return lhs.first < rhs.first;
        }
        return lhs.second > rhs.second;
        });

    std::map<std::string, int> result;
    int added = 0;
    for (const auto& [isbn, count] : ranking) {
        result[isbn] = count;
        ++added;
        if (limit > 0 && added >= limit) {
            break;
        }
    }
    return result;
}

// 计算预约满足率
double ReservationSystem::getReservationFulfillmentRate() const {
    ReservationStats stats = getReservationStatistics();
    if (stats.totalReservations == 0) {
        return 0.0;
    }
    return static_cast<double>(stats.fulfilledReservations) / static_cast<double>(stats.totalReservations);
}

// 设置预约优先级
bool ReservationSystem::setReservationPriority(const std::string& readerId,
    const std::string& isbn,
    PriorityLevel priority) {
    auto it = m_reservationsByISBN.find(isbn);
    if (it == m_reservationsByISBN.end()) {
        return false;
    }
    auto& queue = it->second;
    bool updated = false;

    for (auto& record : queue) {
        if (record.readerId == readerId && record.status == ReservationStatus::Active) {
            record.priority = priority;
            updated = true;
        }
    }
    if (updated) {
        sortReservations(queue);
        rebuildIndex(isbn);
    }
    return updated;
}

// 创建团体预约（批量预约）
bool ReservationSystem::createGroupReservation(const std::vector<std::string>& readerIds, const std::string& isbn) {
    bool success = false;
    for (const auto& readerId : readerIds) {
        ReservationResult res = reserveBook(readerId, isbn);
        success = res.success || success;
    }
    return success;
}

// 转让预约（将名额转给另一位读者）
bool ReservationSystem::transferReservation(const std::string& fromReaderId,
    const std::string& toReaderId,
    const std::string& isbn) {
    std::string error;
    // 检查接收者状态
    auto toReader = m_readerService.getReaderById(toReaderId);
    if (toReader == nullptr) {
        return false;
    }
    if (!toReader->isActive() || toReader->isSuspended()) {
        return false;
    }

    auto it = m_reservationsByISBN.find(isbn);
    if (it == m_reservationsByISBN.end()) {
        return false;
    }

    auto& queue = it->second;
    bool transferred = false;
    for (auto& record : queue) {
        if (record.readerId == fromReaderId && record.status == ReservationStatus::Active) {
            // 确保目标读者不在同一队列中
            for (const auto& other : queue) {
                if (other.readerId == toReaderId && other.status == ReservationStatus::Active) {
                    return false;
                }
            }
            record.readerId = toReaderId;
            transferred = true;
            break;
        }
    }
    if (transferred) {
        sortReservations(queue);
        rebuildIndex(isbn);
    }
    return transferred;
}

// 生成唯一预约ID
std::string ReservationSystem::generateReservationId() {
    ++m_sequence;
    std::ostringstream oss;
    oss << "RSV-" << m_sequence;
    return oss.str();
}

// 重建索引（ReservationId -> ISBN, Index）
void ReservationSystem::rebuildIndex(const std::string& isbn) {
    auto it = m_reservationsByISBN.find(isbn);
    if (it == m_reservationsByISBN.end()) {
        return;
    }
    auto& queue = it->second;
    for (std::size_t i = 0; i < queue.size(); ++i) {
        m_indexById[queue[i].reservationId] = { isbn, i };
    }
}

// 辅助函数：通过 ID 查找预约记录的位置
std::optional<std::pair<std::string, std::size_t>> ReservationSystem::locateReservation(const std::string& reservationId) {
    auto it = m_indexById.find(reservationId);
    if (it == m_indexById.end()) {
        return std::nullopt;
    }
    return it->second;
}

// 检查预约前置条件
bool ReservationSystem::ensureReservationPrerequisites(const std::string& readerId,
    const std::string& isbn,
    std::string& error) const {
    // 1. 检查读者
    const Reader* reader = m_readerService.getReaderById(readerId);
    if (reader == nullptr) {
        error = "读者不存在。";
        return false;
    }
    if (!reader->isActive()) {
        error = "读者账户未激活。";
        return false;
    }
    if (reader->isSuspended()) {
        error = "读者账户已被暂停。";
        return false;
    }

    // 2. 检查图书
    auto books = m_bookService.searchByISBN(isbn);
    if (books.empty()) {
        error = "图书不存在。";
        return false;
    }

    // 3. 检查是否有库存可直接借阅（无需预约）
    if (m_borrowService.checkBorrowEligibility(readerId, isbn)) {
        error = "图书当前可借，无需预约。";
        return false;
    }

    // 4. 检查是否重复预约
    auto it = m_reservationsByISBN.find(isbn);
    if (it != m_reservationsByISBN.end()) {
        for (const auto& record : it->second) {
            if (record.readerId == readerId && record.status == ReservationStatus::Active) {
                error = "该读者已在预约队列中。";
                return false;
            }
        }
    }

    error.clear();
    return true;
}

// 对指定队列排序并重建索引
void ReservationSystem::sortQueue(std::vector<ReservationRecord>& queue) {
    sortReservations(queue);
    if (!queue.empty()) {
        const std::string& isbn = queue.front().isbn;
        rebuildIndex(isbn);
    }
}

