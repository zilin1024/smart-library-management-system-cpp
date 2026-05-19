#include "Reader.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

// ==== 构造函数 =============================================================

// 默认构造函数：初始化基础字段
Reader::Reader()
    : m_contactInfo{},
    m_age(0),
    m_maxBorrow(5),
    m_active(true),
    m_suspended(false),
    m_creditScore(600),
    m_registerDate(DateUtil::today()),
    m_lastActiveDate(DateUtil::today()),
    m_totalBorrowed(0) {}

// 带参构造函数：初始化读者详细信息
Reader::Reader(std::string readerId,
    std::string name,
    ContactInfo contact,
    int maxBorrow,
    int age)
    : m_readerId(std::move(readerId)),
    m_name(std::move(name)),
    m_contactInfo(std::move(contact)),
    m_age(age),
    m_maxBorrow(maxBorrow),
    m_active(true),
    m_suspended(false),
    m_creditScore(600),
    m_registerDate(DateUtil::today()),
    m_lastActiveDate(DateUtil::today()),
    m_totalBorrowed(0) {
    // 校验最大借阅数是否合法
    if (maxBorrow <= 0) {
        throw std::invalid_argument("Max borrow must be positive.");
    }
}

// ==== 基本信息访问 =========================================================

const std::string& Reader::getReaderId() const noexcept {
    return m_readerId;
}

void Reader::setReaderId(const std::string& readerId) {
    m_readerId = readerId;
}

const std::string& Reader::getName() const noexcept {
    return m_name;
}

void Reader::setName(const std::string& name) {
    m_name = name;
}

const ContactInfo& Reader::getContactInfo() const noexcept {
    return m_contactInfo;
}

void Reader::setContactInfo(const ContactInfo& contact) {
    m_contactInfo = contact;
}

int Reader::getAge() const noexcept {
    return m_age;
}

// 设置年龄，需非负
void Reader::setAge(int age) {
    if (age < 0) {
        throw std::invalid_argument("Age cannot be negative.");
    }
    m_age = age;
}

int Reader::getMaxBorrow() const noexcept {
    return m_maxBorrow;
}

// 设置最大借阅数，需为正数
void Reader::setMaxBorrow(int maxBorrow) {
    if (maxBorrow <= 0) {
        throw std::invalid_argument("Max borrow must be positive.");
    }
    m_maxBorrow = maxBorrow;
}

// ==== 账户状态 =============================================================

bool Reader::isActive() const noexcept {
    return m_active;
}

void Reader::activate() {
    m_active = true;
}

void Reader::deactivate() {
    m_active = false;
}

bool Reader::isSuspended() const noexcept {
    return m_suspended;
}

const Date& Reader::getSuspensionLiftDate() const noexcept {
    return m_suspensionLiftDate;
}

// 暂停账户直到指定日期
void Reader::suspendUntil(const Date& liftDate) {
    m_suspended = true;
    m_suspensionLiftDate = liftDate;
}

// 解除停用状态
void Reader::clearSuspension() {
    m_suspended = false;
    m_suspensionLiftDate = Date();
}

int Reader::getCreditScore() const noexcept {
    return m_creditScore;
}

// 直接设置信用分，确保不小于0
void Reader::setCreditScore(int score) {
    m_creditScore = std::max(0, score);
}

// 调整信用分（增减），确保结果非负
void Reader::adjustCreditScore(int delta) {
    m_creditScore = std::max(0, m_creditScore + delta);
}

// ==== 借阅统计 =============================================================

int Reader::getCurrentBorrowCount() const noexcept {
    return static_cast<int>(m_currentBorrowedIsbns.size());
}

int Reader::getTotalBorrowed() const noexcept {
    return m_totalBorrowed;
}

const std::vector<std::string>& Reader::getCurrentBorrowedIsbns() const noexcept {
    return m_currentBorrowedIsbns;
}

const std::vector<std::string>& Reader::getBorrowHistory() const noexcept {
    return m_borrowHistory;
}

// 添加当前借阅
bool Reader::addCurrentBorrow(const std::string& isbn) {
    // 检查是否重复借阅同一本书
    if (std::find(m_currentBorrowedIsbns.begin(), m_currentBorrowedIsbns.end(), isbn) != m_currentBorrowedIsbns.end()) {
        return false;
    }
    // 检查是否达到借阅上限
    if (getCurrentBorrowCount() >= m_maxBorrow) {
        return false;
    }
    m_currentBorrowedIsbns.push_back(isbn);
    ++m_totalBorrowed; // 增加历史累计借阅数
    updateLastActiveDate(DateUtil::today()); // 更新活跃时间
    addBorrowHistoryEntry(isbn); // 记入历史
    return true;
}

// 移除当前借阅（还书时调用）
bool Reader::removeCurrentBorrow(const std::string& isbn) {
    auto it = std::find(m_currentBorrowedIsbns.begin(), m_currentBorrowedIsbns.end(), isbn);
    if (it == m_currentBorrowedIsbns.end()) {
        return false;
    }
    m_currentBorrowedIsbns.erase(it);
    updateLastActiveDate(DateUtil::today());
    return true;
}

void Reader::addBorrowHistoryEntry(const std::string& isbn) {
    m_borrowHistory.push_back(isbn);
}

// ==== 预约与偏好 ===========================================================

const std::vector<std::string>& Reader::getReservations() const noexcept {
    return m_reservations;
}

// 添加预约
void Reader::addReservation(const std::string& isbn) {
    if (std::find(m_reservations.begin(), m_reservations.end(), isbn) == m_reservations.end()) {
        m_reservations.push_back(isbn);
    }
}

// 移除预约
void Reader::removeReservation(const std::string& isbn) {
    auto it = std::remove(m_reservations.begin(), m_reservations.end(), isbn);
    if (it != m_reservations.end()) {
        m_reservations.erase(it, m_reservations.end());
    }
}

void Reader::clearReservations() {
    m_reservations.clear();
}

// ==== 活跃时间 =============================================================

const Date& Reader::getRegisterDate() const noexcept {
    return m_registerDate;
}

const Date& Reader::getLastActiveDate() const noexcept {
    return m_lastActiveDate;
}

void Reader::updateLastActiveDate(const Date& date) {
    m_lastActiveDate = date;
}

// ==== 数据更新 =============================================================

// 应用批量更新，仅当 Optional 有值时修改对应字段
void Reader::applyUpdate(const ReaderUpdate& update) {
    if (update.name.has_value()) {
        setName(update.name.value());
    }
    if (update.contact.has_value()) {
        setContactInfo(update.contact.value());
    }
    if (update.maxBorrow.has_value()) {
        setMaxBorrow(update.maxBorrow.value());
    }
    if (update.age.has_value()) {
        setAge(update.age.value());
    }
}

// ==== 序列化支持 ===========================================================

// 序列化为字符串，列表使用逗号分隔，字段使用 delimiter 分隔
std::string Reader::serialize(char delimiter) const {
    std::ostringstream oss;
    oss << m_readerId << delimiter
        << m_name << delimiter
        << m_contactInfo.phone << delimiter
        << m_contactInfo.email << delimiter
        << m_contactInfo.address << delimiter
        << m_age << delimiter
        << m_maxBorrow << delimiter
        << (m_active ? 1 : 0) << delimiter
        << (m_suspended ? 1 : 0) << delimiter
        << DateUtil::toString(m_suspensionLiftDate) << delimiter
        << m_creditScore << delimiter
        << DateUtil::toString(m_registerDate) << delimiter
        << DateUtil::toString(m_lastActiveDate) << delimiter
        << m_totalBorrowed << delimiter;

    oss << StringUtil::join(m_currentBorrowedIsbns, ",") << delimiter;
    oss << StringUtil::join(m_borrowHistory, ",") << delimiter;
    oss << StringUtil::join(m_reservations, ",");
    return oss.str();
}

// 从字符串反序列化为 Reader 对象
Reader Reader::deserialize(const std::string& line, char delimiter) {
    auto tokens = StringUtil::split(line, delimiter);
    if (tokens.size() < 16) {
        throw std::runtime_error("Invalid serialized reader format.");
    }

    ContactInfo contact{
        tokens[2],
        tokens[3],
        tokens[4]
    };

    Reader reader(tokens[0], tokens[1], contact, std::stoi(tokens[6]), std::stoi(tokens[5]));
    reader.m_active = tokens[7] == "1";
    reader.m_suspended = tokens[8] == "1";
    reader.m_suspensionLiftDate = DateUtil::fromString(tokens[9]);
    reader.m_creditScore = std::stoi(tokens[10]);
    reader.m_registerDate = DateUtil::fromString(tokens[11]);
    reader.m_lastActiveDate = DateUtil::fromString(tokens[12]);
    reader.m_totalBorrowed = std::stoi(tokens[13]);

    // 解析逗号分隔的列表
    reader.m_currentBorrowedIsbns = tokens[14].empty() ? std::vector<std::string>() : StringUtil::split(tokens[14], ',');
    reader.m_borrowHistory = tokens[15].empty() ? std::vector<std::string>() : StringUtil::split(tokens[15], ',');

    if (tokens.size() > 16) {
        reader.m_reservations = tokens[16].empty() ? std::vector<std::string>() : StringUtil::split(tokens[16], ',');
    }

    return reader;
}
