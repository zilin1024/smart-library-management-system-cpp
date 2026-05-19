#include "FileManager.h"
#include "DateUtil.h"
#include "StringUtil.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
    // 定义默认的数据文件名
    constexpr const char* BOOKS_FILE_NAME = "books.dat";
    constexpr const char* READERS_FILE_NAME = "readers.dat";
    constexpr const char* BORROW_FILE_NAME = "borrow_records.dat";

    // 生成用于备份文件夹的时间戳字符串 (格式: YYYYMMDD_HHMMSS)
    std::string makeTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t raw = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &raw);
#else
        localtime_r(&raw, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        return oss.str();
    }

    // CSV转义：处理包含逗号、引号或换行符的字段
    std::string escapeCsv(const std::string& value) {
        bool needQuotes = value.find_first_of(",\"\n\r") != std::string::npos;
        std::string escaped = value;
        StringUtil::replaceAll(escaped, "\"", "\"\""); // 将内部的双引号转义为两个双引号
        if (needQuotes) {
            return "\"" + escaped + "\""; // 用双引号包裹整个字段
        }
        return escaped;
    }

    // 解析CSV行：正确处理引号包裹的字段
    std::vector<std::string> parseCsvLine(const std::string& line) {
        std::vector<std::string> result;
        std::string current;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char ch = line[i];
            if (inQuotes) {
                if (ch == '"') {
                    // 处理转义的双引号
                    if (i + 1 < line.size() && line[i + 1] == '"') {
                        current.push_back('"');
                        ++i;
                    }
                    else {
                        inQuotes = false; // 引号结束
                    }
                }
                else {
                    current.push_back(ch);
                }
            }
            else {
                if (ch == '"') {
                    inQuotes = true; // 引号开始
                }
                else if (ch == ',') {
                    result.push_back(current); // 字段分隔符
                    current.clear();
                }
                else {
                    current.push_back(ch);
                }
            }
        }
        result.push_back(current); // 添加最后一个字段
        return result;
    }

    // JSON转义：处理特殊字符以生成合法的JSON字符串
    std::string jsonEscape(const std::string& value) {
        std::ostringstream oss;
        for (char ch : value) {
            switch (ch) {
            case '\"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                // 处理控制字符
                if (static_cast<unsigned char>(ch) < 0x20) {
                    oss << "\\u" << std::hex << std::uppercase << std::setw(4)
                        << std::setfill('0') << static_cast<int>(ch);
                }
                else {
                    oss << ch;
                }
            }
        }
        return oss.str();
    }
} // namespace

// 构造函数：初始化数据目录和备份目录
FileManager::FileManager(LibraryManager& library, std::filesystem::path dataDir)
    : m_library(library),
    m_dataDirectory(std::move(dataDir)),
    m_backupDirectory(m_dataDirectory / "backups") {
    ensureDirectoryExists(m_dataDirectory);
    ensureDirectoryExists(m_backupDirectory);
}

// ==== 公共接口：文件存储 ====================================================

// 保存图书数据到文件
bool FileManager::saveBooksToFile(const std::string& filename) {
    if (!ensureDirectoryExists(m_dataDirectory)) {
        return false;
    }

    std::vector<std::string> lines;
    auto books = m_library.getAllBooks();
    lines.reserve(books.size());
    for (const auto& book : books) {
        lines.emplace_back(book.serialize()); // 序列化每本图书对象
    }
    return writeLinesToFile(resolveDataFile(filename), lines);
}

// 保存读者数据到文件
bool FileManager::saveReadersToFile(const std::string& filename) {
    if (!ensureDirectoryExists(m_dataDirectory)) {
        return false;
    }

    std::vector<std::string> lines;
    auto readers = m_library.getAllReaders();
    lines.reserve(readers.size());
    for (const auto& reader : readers) {
        lines.emplace_back(reader.serialize());
    }
    return writeLinesToFile(resolveDataFile(filename), lines);
}

// 保存借阅记录到文件
bool FileManager::saveBorrowRecordsToFile(const std::string& filename) {
    if (!ensureDirectoryExists(m_dataDirectory)) {
        return false;
    }

    std::vector<std::string> lines;
    auto records = m_library.getAllBorrowRecords();
    lines.reserve(records.size());
    for (const auto& record : records) {
        lines.emplace_back(record.serialize());
    }
    return writeLinesToFile(resolveDataFile(filename), lines);
}

// 保存所有数据（图书、读者、记录）
bool FileManager::saveAllData() {
    bool ok = true;
    ok = ok && saveBooks();
    ok = ok && saveReaders();
    ok = ok && saveBorrowRecords();
    return ok;
}

// ==== 公共接口：数据加载 ====================================================

// 从文件加载图书数据
bool FileManager::loadBooksFromFile(const std::string& filename) {
    auto path = resolveDataFile(filename);
    if (!std::filesystem::exists(path)) {
        return true; // 首次运行允许文件缺失
    }

    std::vector<std::string> lines;
    if (!readLinesFromFile(path, lines)) {
        return false;
    }

    for (const auto& line : lines) {
        if (line.empty()) {
            continue;
        }
        try {
            Book book = Book::deserialize(line); // 反序列化
            if (!m_library.addBook(book)) {
                // 如果已存在则更新
                m_library.updateBook(book.getISBN(), [&](Book& existing) {
                    existing = book;
                    });
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[FileManager] Failed to load book: " << e.what() << '\n';
            return false;
        }
    }
    return true;
}

// 从文件加载读者数据
bool FileManager::loadReadersFromFile(const std::string& filename) {
    auto path = resolveDataFile(filename);
    if (!std::filesystem::exists(path)) {
        return true;
    }

    std::vector<std::string> lines;
    if (!readLinesFromFile(path, lines)) {
        return false;
    }

    for (const auto& line : lines) {
        if (line.empty()) {
            continue;
        }
        try {
            Reader reader = Reader::deserialize(line);
            if (!m_library.addReader(reader)) {
                m_library.updateReader(reader.getReaderId(), [&](Reader& existing) {
                    existing = reader;
                    });
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[FileManager] Failed to load reader: " << e.what() << '\n';
            return false;
        }
    }
    return true;
}

// 从文件加载借阅记录
bool FileManager::loadBorrowRecordsFromFile(const std::string& filename) {
    auto path = resolveDataFile(filename);
    if (!std::filesystem::exists(path)) {
        return true;
    }

    std::vector<std::string> lines;
    if (!readLinesFromFile(path, lines)) {
        return false;
    }

    for (const auto& line : lines) {
        if (line.empty()) {
            continue;
        }
        try {
            BorrowRecord record = BorrowRecord::deserialize(line);
            if (!m_library.addBorrowRecord(record)) {
                m_library.updateBorrowRecord(record.getRecordId(), [&](BorrowRecord& existing) {
                    existing = record;
                    });
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[FileManager] Failed to load borrow record: " << e.what() << '\n';
            return false;
        }
    }
    return true;
}

// 加载所有数据
bool FileManager::loadAllData() {
    bool ok = true;
    ok = ok && loadBooks();
    ok = ok && loadReaders();
    ok = ok && loadBorrowRecords();
    return ok;
}

// ==== 公共接口：自动备份 ====================================================

// 创建自动备份：复制当前数据文件到带有时间戳的子目录
bool FileManager::createAutoBackup() {
    if (!ensureDirectoryExists(m_backupDirectory)) {
        return false;
    }

    const std::string timestamp = makeTimestamp();
    auto backupFolder = m_backupDirectory / ("backup_" + timestamp);

    try {
        std::filesystem::create_directories(backupFolder);
    }
    catch (const std::exception& e) {
        std::cerr << "[FileManager] Failed to create backup directory: " << e.what() << '\n';
        return false;
    }

    bool ok = true;
    std::vector<std::string> files{ BOOKS_FILE_NAME, READERS_FILE_NAME, BORROW_FILE_NAME };
    for (const auto& filename : files) {
        auto source = resolveDataFile(filename);
        auto destination = backupFolder / filename;
        if (!std::filesystem::exists(source)) {
            continue; // 文件缺失视为正常
        }
        try {
            std::filesystem::copy_file(source, destination,
                std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::exception& e) {
            std::cerr << "[FileManager] Failed to copy file during backup: " << e.what() << '\n';
            ok = false;
        }
    }
    return ok;
}

// 从指定备份恢复数据
bool FileManager::restoreFromBackup(const std::string& backupName) {
    auto backupFolder = resolveBackupFile(backupName);
    if (!std::filesystem::exists(backupFolder) || !std::filesystem::is_directory(backupFolder)) {
        return false;
    }

    bool ok = true;
    std::vector<std::string> files{ BOOKS_FILE_NAME, READERS_FILE_NAME, BORROW_FILE_NAME };

    // 复制备份文件回数据目录
    for (const auto& filename : files) {
        auto source = backupFolder / filename;
        auto destination = resolveDataFile(filename);
        if (!std::filesystem::exists(source)) {
            continue;
        }
        try {
            std::filesystem::copy_file(source, destination,
                std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::exception& e) {
            std::cerr << "[FileManager] Failed to restore file: " << e.what() << '\n';
            ok = false;
        }
    }

    // 恢复成功后重新加载数据到内存
    if (ok) {
        m_library.clearAll();
        ok = loadAllData();
    }
    return ok;
}

// 列出所有可用的备份目录
std::vector<std::string> FileManager::listAvailableBackups() const {
    std::vector<std::string> backups;
    if (!std::filesystem::exists(m_backupDirectory)) {
        return backups;
    }
    for (const auto& entry : std::filesystem::directory_iterator(m_backupDirectory)) {
        if (entry.is_directory()) {
            backups.push_back(entry.path().filename().string());
        }
    }
    std::sort(backups.begin(), backups.end());
    return backups;
}

// ==== 公共接口：导入导出 ====================================================

// 导出所有数据到单个CSV文件
bool FileManager::exportToCSV(const std::string& filename) {
    if (!ensureDirectoryExists(m_dataDirectory)) {
        return false;
    }
    auto path = resolveDataFile(filename);
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        return false;
    }

    ofs << "entity,payload\n"; // 写入CSV头

    // 导出图书
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        ofs << "book," << escapeCsv(book.serialize()) << "\n";
    }

    // 导出读者
    auto readers = m_library.getAllReaders();
    for (const auto& reader : readers) {
        ofs << "reader," << escapeCsv(reader.serialize()) << "\n";
    }

    // 导出借阅记录
    auto records = m_library.getAllBorrowRecords();
    for (const auto& record : records) {
        ofs << "borrow," << escapeCsv(record.serialize()) << "\n";
    }

    return true;
}

// 从CSV文件导入数据
bool FileManager::importFromCSV(const std::string& filename) {
    auto path = resolveDataFile(filename);
    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }

    std::string line;
    bool firstLine = true;
    while (std::getline(ifs, line)) {
        if (firstLine) {
            firstLine = false;
            continue; // 跳过标题行
        }
        if (StringUtil::trim(line).empty()) {
            continue;
        }

        auto columns = parseCsvLine(line);
        if (columns.size() < 2) {
            continue;
        }
        const std::string& entity = columns[0];
        const std::string& payload = columns[1];

        // 根据实体类型进行反序列化并添加
        try {
            if (entity == "book") {
                Book book = Book::deserialize(payload);
                if (!m_library.addBook(book)) {
                    m_library.updateBook(book.getISBN(), [&](Book& existing) {
                        existing = book;
                        });
                }
            }
            else if (entity == "reader") {
                Reader reader = Reader::deserialize(payload);
                if (!m_library.addReader(reader)) {
                    m_library.updateReader(reader.getReaderId(), [&](Reader& existing) {
                        existing = reader;
                        });
                }
            }
            else if (entity == "borrow") {
                BorrowRecord record = BorrowRecord::deserialize(payload);
                if (!m_library.addBorrowRecord(record)) {
                    m_library.updateBorrowRecord(record.getRecordId(), [&](BorrowRecord& existing) {
                        existing = record;
                        });
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[FileManager] Failed to import CSV line: " << e.what() << '\n';
        }
    }

    return true;
}

// 导出所有数据到JSON格式
bool FileManager::exportToJSON(const std::string& filename) {
    if (!ensureDirectoryExists(m_dataDirectory)) {
        return false;
    }
    auto path = resolveDataFile(filename);
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        return false;
    }

    ofs << "{\n";

    // 导出图书数组
    ofs << "  \"books\": [\n";
    auto books = m_library.getAllBooks();
    for (size_t i = 0; i < books.size(); ++i) {
        const auto& book = books[i];
        ofs << "    {\n";
        ofs << "      \"isbn\": \"" << jsonEscape(book.getISBN()) << "\",\n";
        ofs << "      \"title\": \"" << jsonEscape(book.getTitle()) << "\",\n";
        ofs << "      \"author\": \"" << jsonEscape(book.getAuthor()) << "\",\n";
        ofs << "      \"publisher\": \"" << jsonEscape(book.getPublisher()) << "\",\n";
        ofs << "      \"publishYear\": " << book.getPublishYear() << ",\n";
        ofs << "      \"category\": \"" << jsonEscape(book.getCategory()) << "\",\n";
        ofs << "      \"stock\": " << book.getStock() << ",\n";
        ofs << "      \"totalBorrowed\": " << book.getTotalBorrowed() << ",\n";
        ofs << "      \"averageRating\": " << std::fixed << std::setprecision(2) << book.getAverageRating() << ",\n";
        ofs << "      \"ratingCount\": " << book.getRatingCount() << ",\n";
        ofs << "      \"addedDate\": \"" << jsonEscape(DateUtil::toString(book.getAddedDate())) << "\",\n";
        ofs << "      \"lastUpdated\": \"" << jsonEscape(DateUtil::toString(book.getLastUpdatedDate())) << "\",\n";
        ofs << "      \"tags\": [";
        const auto& tags = book.getTags();
        for (size_t t = 0; t < tags.size(); ++t) {
            ofs << "\"" << jsonEscape(tags[t]) << "\"";
            if (t + 1 < tags.size()) {
                ofs << ", ";
            }
        }
        ofs << "]\n";
        ofs << "    }";
        if (i + 1 < books.size()) {
            ofs << ",";
        }
        ofs << "\n";
    }
    ofs << "  ],\n";

    // 导出读者数组
    ofs << "  \"readers\": [\n";
    auto readers = m_library.getAllReaders();
    for (size_t i = 0; i < readers.size(); ++i) {
        const auto& reader = readers[i];
        ofs << "    {\n";
        ofs << "      \"readerId\": \"" << jsonEscape(reader.getReaderId()) << "\",\n";
        ofs << "      \"name\": \"" << jsonEscape(reader.getName()) << "\",\n";
        ofs << "      \"contact\": {\n";
        ofs << "        \"phone\": \"" << jsonEscape(reader.getContactInfo().phone) << "\",\n";
        ofs << "        \"email\": \"" << jsonEscape(reader.getContactInfo().email) << "\",\n";
        ofs << "        \"address\": \"" << jsonEscape(reader.getContactInfo().address) << "\"\n";
        ofs << "      },\n";
        ofs << "      \"age\": " << reader.getAge() << ",\n";
        ofs << "      \"maxBorrow\": " << reader.getMaxBorrow() << ",\n";
        ofs << "      \"active\": " << (reader.isActive() ? "true" : "false") << ",\n";
        ofs << "      \"suspended\": " << (reader.isSuspended() ? "true" : "false") << ",\n";
        ofs << "      \"suspensionLiftDate\": \"" << jsonEscape(DateUtil::toString(reader.getSuspensionLiftDate())) << "\",\n";
        ofs << "      \"creditScore\": " << reader.getCreditScore() << ",\n";
        ofs << "      \"registerDate\": \"" << jsonEscape(DateUtil::toString(reader.getRegisterDate())) << "\",\n";
        ofs << "      \"lastActiveDate\": \"" << jsonEscape(DateUtil::toString(reader.getLastActiveDate())) << "\",\n";
        ofs << "      \"totalBorrowed\": " << reader.getTotalBorrowed() << ",\n";
        ofs << "      \"currentBorrowedIsbns\": [";
        const auto& currentIsbns = reader.getCurrentBorrowedIsbns();
        for (size_t c = 0; c < currentIsbns.size(); ++c) {
            ofs << "\"" << jsonEscape(currentIsbns[c]) << "\"";
            if (c + 1 < currentIsbns.size()) {
                ofs << ", ";
            }
        }
        ofs << "],\n";
        ofs << "      \"borrowHistory\": [";
        const auto& history = reader.getBorrowHistory();
        for (size_t h = 0; h < history.size(); ++h) {
            ofs << "\"" << jsonEscape(history[h]) << "\"";
            if (h + 1 < history.size()) {
                ofs << ", ";
            }
        }
        ofs << "],\n";
        ofs << "      \"reservations\": [";
        const auto& reservations = reader.getReservations();
        for (size_t r = 0; r < reservations.size(); ++r) {
            ofs << "\"" << jsonEscape(reservations[r]) << "\"";
            if (r + 1 < reservations.size()) {
                ofs << ", ";
            }
        }
        ofs << "]\n";
        ofs << "    }";
        if (i + 1 < readers.size()) {
            ofs << ",";
        }
        ofs << "\n";
    }
    ofs << "  ],\n";

    // 导出借阅记录数组
    ofs << "  \"borrowRecords\": [\n";
    auto records = m_library.getAllBorrowRecords();
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& record = records[i];
        ofs << "    {\n";
        ofs << "      \"recordId\": \"" << jsonEscape(record.getRecordId()) << "\",\n";
        ofs << "      \"readerId\": \"" << jsonEscape(record.getReaderId()) << "\",\n";
        ofs << "      \"isbn\": \"" << jsonEscape(record.getISBN()) << "\",\n";
        ofs << "      \"borrowDate\": \"" << jsonEscape(DateUtil::toString(record.getBorrowDate())) << "\",\n";
        ofs << "      \"dueDate\": \"" << jsonEscape(DateUtil::toString(record.getDueDate())) << "\",\n";
        ofs << "      \"returnDate\": ";
        if (record.getReturnDate().has_value()) {
            ofs << "\"" << jsonEscape(DateUtil::toString(record.getReturnDate().value())) << "\",\n";
        }
        else {
            ofs << "null,\n";
        }
        ofs << "      \"renewCount\": " << record.getRenewCount() << ",\n";
        ofs << "      \"fine\": " << std::fixed << std::setprecision(2) << record.getFine() << "\n";
        ofs << "    }";
        if (i + 1 < records.size()) {
            ofs << ",";
        }
        ofs << "\n";
    }
    ofs << "  ]\n";
    ofs << "}\n";

    return true;
}

// ==== 公共接口：数据完整性检查 ===============================================

// 校验数据逻辑一致性（如引用是否存在，日期是否合法）
bool FileManager::validateDataIntegrity() {
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        if (book.getISBN().empty() || book.getStock() < 0) {
            return false;
        }
    }

    auto readers = m_library.getAllReaders();
    for (const auto& reader : readers) {
        if (reader.getReaderId().empty() || reader.getMaxBorrow() <= 0) {
            return false;
        }
    }

    auto records = m_library.getAllBorrowRecords();
    for (const auto& record : records) {
        if (record.getRecordId().empty()) {
            return false;
        }
        if (m_library.getReader(record.getReaderId()) == nullptr) {
            return false; // 记录引用的读者不存在
        }
        if (m_library.getBook(record.getISBN()) == nullptr) {
            return false; // 记录引用的图书不存在
        }
        if (record.getDueDate() < record.getBorrowDate()) {
            return false; // 到期日在借阅日之前
        }
        if (record.getReturnDate().has_value() &&
            record.getReturnDate().value() < record.getBorrowDate()) {
            return false; // 归还日在借阅日之前
        }
        if (record.getFine() < 0.0) {
            return false;
        }
    }

    return true;
}

// 尝试修复损坏的数据
bool FileManager::repairCorruptedData() {
    bool repaired = false;

    // 修复图书库存：负数重置为0
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        if (book.getStock() < 0) {
            m_library.updateBook(book.getISBN(), [](Book& mutableBook) {
                mutableBook.setStock(0);
                });
            repaired = true;
        }
    }

    // 修复读者最大借阅数：无效值重置为1
    auto readers = m_library.getAllReaders();
    for (const auto& reader : readers) {
        if (reader.getMaxBorrow() <= 0) {
            m_library.updateReader(reader.getReaderId(), [](Reader& mutableReader) {
                mutableReader.setMaxBorrow(1);
                });
            repaired = true;
        }
    }

    // 移除引用失效或日期逻辑错误的借阅记录
    auto records = m_library.getAllBorrowRecords();
    for (const auto& record : records) {
        bool invalid = false;
        if (m_library.getReader(record.getReaderId()) == nullptr ||
            m_library.getBook(record.getISBN()) == nullptr) {
            invalid = true;
        }
        if (record.getDueDate() < record.getBorrowDate()) {
            invalid = true;
        }
        if (record.getReturnDate().has_value() &&
            record.getReturnDate().value() < record.getBorrowDate()) {
            invalid = true;
        }
        if (invalid) {
            m_library.removeBorrowRecord(record.getRecordId());
            repaired = true;
        }
    }

    return repaired;
}

// ==== 公共接口：目录配置 ====================================================

void FileManager::setDataDirectory(const std::filesystem::path& path) {
    m_dataDirectory = path;
    m_backupDirectory = m_dataDirectory / "backups";
    ensureDirectoryExists(m_dataDirectory);
    ensureDirectoryExists(m_backupDirectory);
}

std::filesystem::path FileManager::getDataDirectory() const {
    return m_dataDirectory;
}

std::filesystem::path FileManager::getBackupDirectory() const {
    return m_backupDirectory;
}

// ==== 私有工具函数 ==========================================================

// 确保目录存在，若不存在则创建
bool FileManager::ensureDirectoryExists(const std::filesystem::path& path) const {
    try {
        if (std::filesystem::exists(path)) {
            return true;
        }
        return std::filesystem::create_directories(path);
    }
    catch (const std::exception& e) {
        std::cerr << "[FileManager] Failed to ensure directory: " << e.what() << '\n';
        return false;
    }
}

// 获取完整数据文件路径
std::filesystem::path FileManager::resolveDataFile(const std::string& filename) const {
    return m_dataDirectory / filename;
}

// 获取完整备份路径
std::filesystem::path FileManager::resolveBackupFile(const std::string& name) const {
    return m_backupDirectory / name;
}

// 辅助函数：将字符串列表写入文件
bool FileManager::writeLinesToFile(const std::filesystem::path& filepath,
    const std::vector<std::string>& lines) {
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        return false;
    }
    for (const auto& line : lines) {
        ofs << line << "\n";
    }
    return true;
}

// 辅助函数：从文件读取所有行
bool FileManager::readLinesFromFile(const std::filesystem::path& filepath,
    std::vector<std::string>& outLines) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        return false;
    }
    outLines.clear();
    std::string line;
    while (std::getline(ifs, line)) {
        outLines.push_back(line);
    }
    return true;
}

// ==== 默认文件存取 ==========================================================

// 使用默认文件名保存
bool FileManager::saveBooks() {
    return saveBooksToFile(BOOKS_FILE_NAME);
}

bool FileManager::saveReaders() {
    return saveReadersToFile(READERS_FILE_NAME);
}

bool FileManager::saveBorrowRecords() {
    return saveBorrowRecordsToFile(BORROW_FILE_NAME);
}

// 使用默认文件名加载
bool FileManager::loadBooks() {
    return loadBooksFromFile(BOOKS_FILE_NAME);
}

bool FileManager::loadReaders() {
    return loadReadersFromFile(READERS_FILE_NAME);
}

bool FileManager::loadBorrowRecords() {
    return loadBorrowRecordsFromFile(BORROW_FILE_NAME);
}

// 辅助导出CSV
bool FileManager::exportBooksToCSV(std::ostream& os) {
    auto books = m_library.getAllBooks();
    for (const auto& book : books) {
        os << "book," << escapeCsv(book.serialize()) << "\n";
    }
    return true;
}

bool FileManager::exportReadersToCSV(std::ostream& os) {
    auto readers = m_library.getAllReaders();
    for (const auto& reader : readers) {
        os << "reader," << escapeCsv(reader.serialize()) << "\n";
    }
    return true;
}

bool FileManager::exportBorrowRecordsToCSV(std::ostream& os) {
    auto records = m_library.getAllBorrowRecords();
    for (const auto& record : records) {
        os << "borrow," << escapeCsv(record.serialize()) << "\n";
    }
    return true;
}

// JSON辅助导出
bool FileManager::exportBooksToJSON(std::ostream& os) {
    (void)os;
    return false;
}

bool FileManager::exportReadersToJSON(std::ostream& os) {
    (void)os;
    return false;
}

bool FileManager::exportBorrowRecordsToJSON(std::ostream& os) {
    (void)os;
    return false;
}

