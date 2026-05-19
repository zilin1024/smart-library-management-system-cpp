#include "Logger.h"
#include <iostream>

// 获取 Logger 的单例实例
Logger& Logger::instance() {
    static Logger logger; // C++11 保证静态局部变量初始化的线程安全性
    return logger;
}

// 默认构造函数
Logger::Logger() = default;

// 析构函数：确保文件流被正确关闭
Logger::~Logger() {
    closeFile();
}

// 设置日志级别（线程安全）
void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

// 启用或禁用控制台输出（线程安全）
void Logger::enableConsoleOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consoleEnabled = enabled;
}

// 打开日志文件（线程安全）
bool Logger::openFile(const std::string& filename, bool append) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ios_base::openmode mode = std::ios::out;
    // 根据 append 参数决定是追加还是截断（覆盖）
    mode |= append ? std::ios::app : std::ios::trunc;
    m_fileStream.open(filename, mode);
    return m_fileStream.is_open();
}

// 关闭日志文件（线程安全）
void Logger::closeFile() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

// 通用日志记录接口
void Logger::log(LogLevel level, const std::string& message) {
    write(level, message);
}

// 便捷接口：记录 Debug 级别日志
void Logger::debug(const std::string& message) {
    write(LogLevel::Debug, message);
}

// 便捷接口：记录 Info 级别日志
void Logger::info(const std::string& message) {
    write(LogLevel::Info, message);
}

// 便捷接口：记录 Warning 级别日志
void Logger::warn(const std::string& message) {
    write(LogLevel::Warning, message);
}

// 便捷接口：记录 Error 级别日志
void Logger::error(const std::string& message) {
    write(LogLevel::Error, message);
}

// 获取当前日志级别
LogLevel Logger::getLogLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

// 生成带毫秒精度的时间戳字符串
std::string Logger::timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    // 计算当前的毫秒部分
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
#if defined(_WIN32) || defined(_WIN64)
    std::tm tm{};
    localtime_s(&tm, &timeT); // Windows 安全版本
#else
    std::tm tm{};
    localtime_r(&timeT, &tm); // Linux/Unix 线程安全版本
#endif
    // 格式化输出：YYYY-MM-DD HH:MM:SS.mmm
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// 将日志级别枚举转换为字符串
std::string Logger::levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    default: return "UNKNOWN";
    }
}

// 核心写入函数：处理格式化、级别过滤和输出（线程安全）
void Logger::write(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex); // 自动加锁，作用域结束自动解锁

    // 级别过滤：如果当前日志级别低于设定的阈值，则忽略
    if (static_cast<int>(level) < static_cast<int>(m_level)) {
        return;
    }

    // 构造日志行：[时间戳] 级别 - 消息
    std::ostringstream oss;
    oss << "[" << timestamp() << "] "
        << levelToString(level) << " - "
        << message << '\n';

    // 输出到控制台
    if (m_consoleEnabled) {
        std::cout << oss.str();
    }
    // 输出到文件
    if (m_fileStream.is_open()) {
        m_fileStream << oss.str();
        m_fileStream.flush(); // 确保立即写入磁盘
    }
}