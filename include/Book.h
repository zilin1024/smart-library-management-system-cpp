#ifndef BOOK_H
#define BOOK_H

#include "DateUtil.h"
#include <string>
#include <vector>

/**
 * @class Book
 * @brief 核心图书实体类，封装一本书的全部元数据与库存信息。
 *
 * 该类位于基础层（Core Layer），为图书管理、借阅管理、
 * 高级分析等服务提供标准化的数据结构。
 */
class Book {
public:
    /**
     * @brief 默认构造函数，生成占位图书对象。
     */
    Book();

    /**
     * @brief 使用完整元数据构造图书对象。
     * @param isbn          国际标准书号，系统内的唯一标识。
     * @param title         书名。
     * @param author        作者。
     * @param publisher     出版社。
     * @param publishYear   出版年份。
     * @param category      图书分类。
     * @param stock         当前可借库存数量。
     */
    Book(std::string isbn,
        std::string title,
        std::string author,
        std::string publisher,
        int publishYear,
        std::string category,
        int stock = 0);

    // === 基本属性访问器 ===
    [[nodiscard]] const std::string& getISBN() const noexcept;
    void setISBN(const std::string& isbn);

    [[nodiscard]] const std::string& getTitle() const noexcept;
    void setTitle(const std::string& title);

    [[nodiscard]] const std::string& getAuthor() const noexcept;
    void setAuthor(const std::string& author);

    [[nodiscard]] const std::string& getPublisher() const noexcept;
    void setPublisher(const std::string& publisher);

    [[nodiscard]] int getPublishYear() const noexcept;
    void setPublishYear(int year);

    [[nodiscard]] const std::string& getCategory() const noexcept;
    void setCategory(const std::string& category);

    [[nodiscard]] const std::vector<std::string>& getTags() const noexcept;
    void setTags(const std::vector<std::string>& tags);
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);

    // === 库存管理 ===
    [[nodiscard]] int getStock() const noexcept;
    void increaseStock(int amount);
    bool decreaseStock(int amount);
    void setStock(int amount);

    [[nodiscard]] int getTotalBorrowed() const noexcept;
    void incrementBorrowedCount();
    void decrementBorrowedCount();

    // === 评分与统计 ===
    [[nodiscard]] double getAverageRating() const noexcept;
    [[nodiscard]] int getRatingCount() const noexcept;
    void addRating(double rating);

    [[nodiscard]] const Date& getAddedDate() const noexcept;
    void setAddedDate(const Date& date);

    [[nodiscard]] const Date& getLastUpdatedDate() const noexcept;
    void setLastUpdatedDate(const Date& date);

    // === 匹配与查询辅助 ===
    [[nodiscard]] bool matchesKeyword(const std::string& keyword) const;
    [[nodiscard]] bool matchesAuthor(const std::string& authorQuery) const;
    [[nodiscard]] bool matchesTitle(const std::string& titleQuery) const;
    [[nodiscard]] bool matchesPublisher(const std::string& publisherQuery) const;
    [[nodiscard]] bool matchesCategory(const std::string& categoryQuery) const;

    // === 序列化（用于文件持久化） ===
    [[nodiscard]] std::string serialize(char delimiter = '|') const;
    static Book deserialize(const std::string& line, char delimiter = '|');

private:
    std::string m_isbn;
    std::string m_title;
    std::string m_author;
    std::string m_publisher;
    int m_publishYear{ 0 };
    std::string m_category;
    int m_stock{ 0 };
    int m_totalBorrowed{ 0 };
    std::vector<std::string> m_tags;

    double m_totalRating{ 0.0 };
    int m_ratingCount{ 0 };

    Date m_addedDate;
    Date m_lastUpdatedDate;
};

#endif // BOOK_H
#pragma once
