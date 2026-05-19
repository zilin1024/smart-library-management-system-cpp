#include "BorrowRecord.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

// ==== 构造函数 =============================================================

// 默认构造函数
BorrowRecord::BorrowRecord()
    : m_recordId(),
    m_readerId(),
    m_isbn(),
    m_borrowDate(DateUtil::today()),
    m_dueDate(DateUtil::addDays(DateUtil::today(), 14)),  // 默认借阅14天
    m_returnDate(),
    m_borrowHour(DateUtil::currentHour()),
    m_renewCount(0),
    m_fine(0.0) {}

// 带参数构造函数
BorrowRecord::BorrowRecord(std::string recordId,
    std::string readerId,
    std::string isbn,
    const Date& borrowDate,
    const Date& dueDate,
    int borrowHour)
    : m_recordId(std::move(recordId)),
    m_readerId(std::move(readerId)),
    m_isbn(std::move(isbn)),
    m_borrowDate(borrowDate),
    m_dueDate(dueDate),
    m_returnDate(),
    m_borrowHour(std::clamp(borrowHour, 0, 23)),  // 限制小时在0-23范围内
    m_renewCount(0),
    m_fine(0.0) {}

// ==== 基本信息访问 =========================================================

// 获取记录ID
const std::string& BorrowRecord::getRecordId() const noexcept {
    return m_recordId;
}

// 设置记录ID
void BorrowRecord::setRecordId(const std::string& recordId) {
    m_recordId = recordId;
}

// 获取读者ID
const std::string& BorrowRecord::getReaderId() const noexcept {
    return m_readerId;
}

// 设置读者ID
void BorrowRecord::setReaderId(const std::string& readerId) {
    m_readerId = readerId;
}

// 获取ISBN
const std::string& BorrowRecord::getISBN() const noexcept {
    return m_isbn;
}

// 设置ISBN
void BorrowRecord::setISBN(const std::string& isbn) {
    m_isbn = isbn;
}

// 获取借阅日期
const Date& BorrowRecord::getBorrowDate() const noexcept {
    return m_borrowDate;
}

// 设置借阅日期
void BorrowRecord::setBorrowDate(const Date& date) {
    m_borrowDate = date;
}

// 获取应还日期
const Date& BorrowRecord::getDueDate() const noexcept {
    return m_dueDate;
}

// 设置应还日期
void BorrowRecord::setDueDate(const Date& date) {
    m_dueDate = date;
}

// 获取归还日期
const std::optional<Date>& BorrowRecord::getReturnDate() const noexcept {
    return m_returnDate;
}

// 设置归还日期
void BorrowRecord::setReturnDate(const Date& date) {
    m_returnDate = date;
}

// 清除归还日期（表示还未归还）
void BorrowRecord::clearReturnDate() {
    m_returnDate.reset();
}

// 获取借阅小时
int BorrowRecord::getBorrowHour() const noexcept {
    return m_borrowHour;
}

// 设置借阅小时
void BorrowRecord::setBorrowHour(int hour) {
    m_borrowHour = std::clamp(hour, 0, 23);
}

// ==== 续借与状态 ===========================================================

// 获取续借次数
int BorrowRecord::getRenewCount() const noexcept {
    return m_renewCount;
}

// 增加续借次数
void BorrowRecord::incrementRenewCount() {
    ++m_renewCount;
}

// 检查是否已归还
bool BorrowRecord::isReturned() const noexcept {
    return m_returnDate.has_value();
}

// 检查是否逾期（相对于指定日期）
bool BorrowRecord::isOverdue(const Date& onDate) const noexcept {
    if (isReturned()) {
        return m_returnDate.value() > m_dueDate;  // 已归还但超过应还日期
    }
    return onDate > m_dueDate;  // 未归还且当前日期超过应还日期
}

// 获取借阅时长（天数）
int BorrowRecord::getBorrowDuration() const noexcept {
    if (m_returnDate.has_value()) {
        return DateUtil::daysBetween(m_borrowDate, m_returnDate.value());  // 已归还，计算实际借阅天数
    }
    return DateUtil::daysBetween(m_borrowDate, DateUtil::today());  // 未归还，计算到今天的借阅天数
}

// 获取逾期天数（相对于指定日期）
int BorrowRecord::getOverdueDays(const Date& onDate) const noexcept {
    if (!isOverdue(onDate)) {
        return 0;  // 未逾期
    }

    if (m_returnDate.has_value()) {
        return DateUtil::daysBetween(m_dueDate, m_returnDate.value());  // 已归还，计算应还日到归还日的逾期天数
    }
    return DateUtil::daysBetween(m_dueDate, onDate);  // 未归还，计算应还日到指定日期的逾期天数
}

// ==== 罚金与统计 ===========================================================

// 获取罚金
double BorrowRecord::getFine() const noexcept {
    return m_fine;
}

// 设置罚金
void BorrowRecord::setFine(double fine) {
    if (fine < 0.0) {
        throw std::invalid_argument("罚金不能为负值。");
    }
    m_fine = fine;
}

// ==== 序列化支持 ===========================================================

// 序列化为字符串
std::string BorrowRecord::serialize(char delimiter) const {
    std::vector<std::string> tokens;
    tokens.emplace_back(m_recordId);
    tokens.emplace_back(m_readerId);
    tokens.emplace_back(m_isbn);
    tokens.emplace_back(DateUtil::toString(m_borrowDate));
    tokens.emplace_back(DateUtil::toString(m_dueDate));
    // 归还日期可能为空
    tokens.emplace_back(m_returnDate.has_value() ? DateUtil::toString(m_returnDate.value()) : "");
    tokens.emplace_back(std::to_string(m_borrowHour));
    tokens.emplace_back(std::to_string(m_renewCount));
    tokens.emplace_back(std::to_string(m_fine));

    // 用分隔符连接所有字段
    std::ostringstream oss;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            oss << delimiter;
        }
        oss << tokens[i];
    }
    return oss.str();
}

// 从字符串反序列化
BorrowRecord BorrowRecord::deserialize(const std::string& line, char delimiter) {
    auto tokens = StringUtil::split(line, delimiter);
    if (tokens.size() < 7) {
        throw std::runtime_error("借阅记录格式不完整，无法解析。");
    }

    BorrowRecord record;
    record.m_recordId = tokens[0];
    record.m_readerId = tokens[1];
    record.m_isbn = tokens[2];
    record.m_borrowDate = DateUtil::fromString(tokens[3]);
    record.m_dueDate = DateUtil::fromString(tokens[4]);

    // 解析归还日期（可选字段）
    if (!tokens[5].empty()) {
        record.m_returnDate = DateUtil::fromString(tokens[5]);
    }

    // 解析可选字段，注意处理可能的异常
    int index = 6;
    if (tokens.size() > index) {
        try {
            record.m_borrowHour = std::clamp(std::stoi(tokens[index]), 0, 23);
        }
        catch (...) {
            record.m_borrowHour = 0;
        }
    }
    ++index;

    // 续借次数
    if (tokens.size() > index) {
        record.m_renewCount = std::stoi(tokens[index]);
    }
    ++index;

    // 罚金
    if (tokens.size() > index) {
        record.m_fine = std::stod(tokens[index]);
    }

    return record;
}
