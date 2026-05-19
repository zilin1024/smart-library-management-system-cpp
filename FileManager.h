#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "Book.h"
#include "BorrowRecord.h"
#include "LibraryManager.h"
#include "Reader.h"

#include <filesystem>
#include <string>
#include <vector>

/**
 * @class FileManager
 * @brief 基础层文件持久化组件，负责系统数据的保存、加载、备份与导入导出。
 *
 * FileManager 依赖 LibraryManager 提供的统一数据访问接口，实现对图书、
 * 读者与借阅记录等核心数据的持久化存储。所有文件操作均采用 UTF-8 文本格式，
 * 并支持 CSV/JSON 等扩展导出能力。
 */
class FileManager {
public:
    explicit FileManager(LibraryManager& library, std::filesystem::path dataDir = "./data");

    // ==== 文件存储 ====
    bool saveBooksToFile(const std::string& filename);
    bool saveReadersToFile(const std::string& filename);
    bool saveBorrowRecordsToFile(const std::string& filename);
    bool saveAllData();

    // ==== 数据加载 ====
    bool loadBooksFromFile(const std::string& filename);
    bool loadReadersFromFile(const std::string& filename);
    bool loadBorrowRecordsFromFile(const std::string& filename);
    bool loadAllData();

    // ==== 自动备份 ====
    bool createAutoBackup();
    bool restoreFromBackup(const std::string& backupName);
    std::vector<std::string> listAvailableBackups() const;

    // ==== 数据导入导出 ====
    bool exportToCSV(const std::string& filename);
    bool importFromCSV(const std::string& filename);
    bool exportToJSON(const std::string& filename);

    // ==== 数据完整性检查 ====
    bool validateDataIntegrity();
    bool repairCorruptedData();

    // ==== 配置 ====
    void setDataDirectory(const std::filesystem::path& path);
    [[nodiscard]] std::filesystem::path getDataDirectory() const;
    [[nodiscard]] std::filesystem::path getBackupDirectory() const;

private:
    LibraryManager& m_library;  // 数据源引用（不拥有所有权）
    std::filesystem::path m_dataDirectory;     // 主数据目录
    std::filesystem::path m_backupDirectory;   // 备份目录（通常是dataDir/backup）

    bool ensureDirectoryExists(const std::filesystem::path& path) const;
    [[nodiscard]] std::filesystem::path resolveDataFile(const std::string& filename) const;
    [[nodiscard]] std::filesystem::path resolveBackupFile(const std::string& name) const;

    bool writeLinesToFile(const std::filesystem::path& filepath, const std::vector<std::string>& lines);
    bool readLinesFromFile(const std::filesystem::path& filepath, std::vector<std::string>& outLines);

    bool saveBooks();
    bool saveReaders();
    bool saveBorrowRecords();

    bool loadBooks();
    bool loadReaders();
    bool loadBorrowRecords();

    bool exportBooksToCSV(std::ostream& os);
    bool exportReadersToCSV(std::ostream& os);
    bool exportBorrowRecordsToCSV(std::ostream& os);

    bool exportBooksToJSON(std::ostream& os);
    bool exportReadersToJSON(std::ostream& os);
    bool exportBorrowRecordsToJSON(std::ostream& os);
};

#endif // FILE_MANAGER_H
