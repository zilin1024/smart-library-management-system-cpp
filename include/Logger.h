#ifndef LOGGER_H
#define LOGGER_H

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>

/**
 * @enum LogLevel
 * @brief 日志级别。
 */
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

/**
 * @class Logger
 * @brief 线程安全的日志记录器，支持控制台与文件输出。
 */
class Logger {
public:
    static Logger& instance();

    void setLogLevel(LogLevel level);
    void enableConsoleOutput(bool enabled);
    bool openFile(const std::string& filename, bool append = true);
    void closeFile();

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    [[nodiscard]] LogLevel getLogLevel() const;

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_level{ LogLevel::Info };
    bool m_consoleEnabled{ true };
    std::ofstream m_fileStream;
    mutable std::mutex m_mutex;

    std::string timestamp() const;
    static std::string levelToString(LogLevel level);
    void write(LogLevel level, const std::string& message);
};

#endif // LOGGER_H
