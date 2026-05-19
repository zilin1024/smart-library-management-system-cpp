#include "Book.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// ==== 构造函数 =============================================================

// 默认构造函数
Book::Book()
    : m_publishYear(0),
    m_stock(0),
    m_totalBorrowed(0),
    m_totalRating(0.0),
    m_ratingCount(0),
    m_addedDate(DateUtil::today()),
    m_lastUpdatedDate(DateUtil::today()) {}

// 带参数构造函数
Book::Book(std::string isbn,
    std::string title,
    std::string author,
    std::string publisher,
    int publishYear,
    std::string category,
    int stock)
    : m_isbn(std::move(isbn)),
    m_title(std::move(title)),
    m_author(std::move(author)),
    m_publisher(std::move(publisher)),
    m_publishYear(publishYear),
    m_category(std::move(category)),
    m_stock(stock),
    m_totalBorrowed(0),
    m_totalRating(0.0),
    m_ratingCount(0),
    m_addedDate(DateUtil::today()),
    m_lastUpdatedDate(DateUtil::today()) {
    if (stock < 0) {
        throw std::invalid_argument("库存不能为负数。");
    }
}

// ==== 基本属性访问器 =======================================================

// 获取ISBN
const std::string& Book::getISBN() const noexcept {
    return m_isbn;
}

// 设置ISBN
void Book::setISBN(const std::string& isbn) {
    m_isbn = isbn;
    m_lastUpdatedDate = DateUtil::today();
}

// 获取标题
const std::string& Book::getTitle() const noexcept {
    return m_title;
}

// 设置标题
void Book::setTitle(const std::string& title) {
    m_title = title;
    m_lastUpdatedDate = DateUtil::today();
}

// 获取作者
const std::string& Book::getAuthor() const noexcept {
    return m_author;
}

// 设置作者
void Book::setAuthor(const std::string& author) {
    m_author = author;
    m_lastUpdatedDate = DateUtil::today();
}

// 获取出版社
const std::string& Book::getPublisher() const noexcept {
    return m_publisher;
}

// 设置出版社
void Book::setPublisher(const std::string& publisher) {
    m_publisher = publisher;
    m_lastUpdatedDate = DateUtil::today();
}

// 获取出版年份
int Book::getPublishYear() const noexcept {
    return m_publishYear;
}

// 设置出版年份
void Book::setPublishYear(int year) {
    m_publishYear = year;
    m_lastUpdatedDate = DateUtil::today();
}

// 获取类别
const std::string& Book::getCategory() const noexcept {
    return m_category;
}

// 设置类别
void Book::setCategory(const std::string& category) {
    m_category = category;
    m_lastUpdatedDate = DateUtil::today();
}

// 获取标签列表
const std::vector<std::string>& Book::getTags() const noexcept {
    return m_tags;
}

// 设置标签列表
void Book::setTags(const std::vector<std::string>& tags) {
    m_tags = tags;
    m_lastUpdatedDate = DateUtil::today();
}

// 添加单个标签
void Book::addTag(const std::string& tag) {
    if (std::find(m_tags.begin(), m_tags.end(), tag) == m_tags.end()) {
        m_tags.push_back(tag);
        m_lastUpdatedDate = DateUtil::today();
    }
}

// 移除单个标签
void Book::removeTag(const std::string& tag) {
    auto it = std::remove(m_tags.begin(), m_tags.end(), tag);
    if (it != m_tags.end()) {
        m_tags.erase(it, m_tags.end());
        m_lastUpdatedDate = DateUtil::today();
    }
}

// ==== 库存管理 =============================================================

// 获取当前库存
int Book::getStock() const noexcept {
    return m_stock;
}

// 增加库存
void Book::increaseStock(int amount) {
    if (amount < 0) {
        throw std::invalid_argument("增加量不能为负数。");
    }
    m_stock += amount;
    m_lastUpdatedDate = DateUtil::today();
}

// 减少库存
bool Book::decreaseStock(int amount) {
    if (amount < 0) {
        throw std::invalid_argument("减少量不能为负数。");
    }
    if (m_stock < amount) {
        return false;  // 库存不足
    }
    m_stock -= amount;
    m_lastUpdatedDate = DateUtil::today();
    return true;
}

// 设置库存
void Book::setStock(int amount) {
    if (amount < 0) {
        throw std::invalid_argument("库存不能设置为负数。");
    }
    m_stock = amount;
    m_lastUpdatedDate = DateUtil::today();
}

// 获取总借阅次数
int Book::getTotalBorrowed() const noexcept {
    return m_totalBorrowed;
}

// 增加借阅次数
void Book::incrementBorrowedCount() {
    ++m_totalBorrowed;
    m_lastUpdatedDate = DateUtil::today();
}

// 减少借阅次数
void Book::decrementBorrowedCount() {
    if (m_totalBorrowed > 0) {
        --m_totalBorrowed;
        m_lastUpdatedDate = DateUtil::today();
    }
}

// ==== 评分与统计 ===========================================================

// 获取平均评分
double Book::getAverageRating() const noexcept {
    if (m_ratingCount == 0) {
        return 0.0;
    }
    return m_totalRating / static_cast<double>(m_ratingCount);
}

// 获取评分次数
int Book::getRatingCount() const noexcept {
    return m_ratingCount;
}

// 添加评分
void Book::addRating(double rating) {
    // 将评分限制在0.0到5.0之间
    if (rating < 0.0) {
        rating = 0.0;
    }
    else if (rating > 5.0) {
        rating = 5.0;
    }
    m_totalRating += rating;
    ++m_ratingCount;
    m_lastUpdatedDate = DateUtil::today();
}

// 获取添加日期
const Date& Book::getAddedDate() const noexcept {
    return m_addedDate;
}

// 设置添加日期
void Book::setAddedDate(const Date& date) {
    m_addedDate = date;
}

// 获取最后更新日期
const Date& Book::getLastUpdatedDate() const noexcept {
    return m_lastUpdatedDate;
}

// 设置最后更新日期
void Book::setLastUpdatedDate(const Date& date) {
    m_lastUpdatedDate = date;
}

// ==== 匹配与查询辅助 =======================================================

// 检查是否匹配关键词
bool Book::matchesKeyword(const std::string& keyword) const {
    if (keyword.empty()) {
        return true;
    }
    return StringUtil::containsIgnoreCase(m_title, keyword) ||
        StringUtil::containsIgnoreCase(m_author, keyword) ||
        StringUtil::containsIgnoreCase(m_publisher, keyword) ||
        StringUtil::containsIgnoreCase(m_category, keyword) ||
        std::any_of(m_tags.begin(), m_tags.end(), [&](const std::string& tag) {
        return StringUtil::containsIgnoreCase(tag, keyword);
            });
}

// 检查是否匹配作者
bool Book::matchesAuthor(const std::string& authorQuery) const {
    return StringUtil::containsIgnoreCase(m_author, authorQuery);
}

// 检查是否匹配标题
bool Book::matchesTitle(const std::string& titleQuery) const {
    return StringUtil::containsIgnoreCase(m_title, titleQuery);
}

// 检查是否匹配出版社
bool Book::matchesPublisher(const std::string& publisherQuery) const {
    return StringUtil::containsIgnoreCase(m_publisher, publisherQuery);
}

// 检查是否匹配类别
bool Book::matchesCategory(const std::string& categoryQuery) const {
    return StringUtil::containsIgnoreCase(m_category, categoryQuery);
}

// ==== 序列化 ===============================================================

// 序列化到字符串
std::string Book::serialize(char delimiter) const {
    std::ostringstream oss;
    oss << m_isbn << delimiter
        << m_title << delimiter
        << m_author << delimiter
        << m_publisher << delimiter
        << m_publishYear << delimiter
        << m_category << delimiter
        << m_stock << delimiter
        << m_totalBorrowed << delimiter
        << m_totalRating << delimiter
        << m_ratingCount << delimiter
        << DateUtil::toString(m_addedDate) << delimiter
        << DateUtil::toString(m_lastUpdatedDate) << delimiter;

    // 将标签列表用逗号连接
    if (!m_tags.empty()) {
        oss << StringUtil::join(m_tags, ",");
    }
    return oss.str();
}

// 从字符串反序列化
Book Book::deserialize(const std::string& line, char delimiter) {
    auto tokens = StringUtil::split(line, delimiter);
    if (tokens.size() < 11) {
        throw std::runtime_error("无效的序列化书籍格式。");
    }

    Book book;
    book.m_isbn = tokens[0];
    book.m_title = tokens[1];
    book.m_author = tokens[2];
    book.m_publisher = tokens[3];
    book.m_publishYear = std::stoi(tokens[4]);
    book.m_category = tokens[5];
    book.m_stock = std::stoi(tokens[6]);
    book.m_totalBorrowed = std::stoi(tokens[7]);
    book.m_totalRating = std::stod(tokens[8]);
    book.m_ratingCount = std::stoi(tokens[9]);
    book.m_addedDate = DateUtil::fromString(tokens[10]);

    // 处理可选字段
    if (tokens.size() > 11) {
        book.m_lastUpdatedDate = DateUtil::fromString(tokens[11]);
    }
    else {
        book.m_lastUpdatedDate = book.m_addedDate;
    }

    // 处理标签
    if (tokens.size() > 12) {
        book.m_tags = StringUtil::split(tokens[12], ',');
    }

    return book;
}
