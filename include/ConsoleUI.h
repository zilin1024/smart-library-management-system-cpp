#pragma once
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX  // 禁止 windows.h 定义 min/max 宏
#endif
#include <windows.h>
#endif



namespace ConsoleUI {

    enum class Color { Default, Primary, Success, Warning, Danger, Muted };

    // ----------- 平台初始化 -----------
    inline void initialize() {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif
    }

    // ----------- 颜色控制 -----------
    inline void applyColor(Color color) {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 默认灰白
        switch (color) {
        case Color::Primary: attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case Color::Success: attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case Color::Warning: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case Color::Danger:  attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        case Color::Muted:   attr = FOREGROUND_INTENSITY; break;
        case Color::Default:
        default: break;
        }
        SetConsoleTextAttribute(hConsole, attr);
#else
        switch (color) {
        case Color::Primary: std::cout << "\033[36m"; break;
        case Color::Success: std::cout << "\033[32m"; break;
        case Color::Warning: std::cout << "\033[33m"; break;
        case Color::Danger:  std::cout << "\033[31m"; break;
        case Color::Muted:   std::cout << "\033[90m"; break;
        case Color::Default:
        default: std::cout << "\033[0m"; break;
        }
#endif
    }

    inline void resetColor() {
        applyColor(Color::Default);
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
        std::cout << "\033[0m";
#endif
    }

    // ----------- 工具函数 -----------
    inline std::size_t calcDisplayWidth(const std::string& text) {
        std::size_t width = 0;
        for (std::size_t i = 0; i < text.size();) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c <= 0x7F) { width += 1; ++i; }
            else if ((c & 0xE0) == 0xC0) { width += 2; i += 2; }
            else if ((c & 0xF0) == 0xE0) { width += 2; i += 3; }
            else if ((c & 0xF8) == 0xF0) { width += 2; i += 4; }
            else { ++width; ++i; }
        }
        return width;
    }

    inline std::string padRight(const std::string& text, std::size_t totalWidth) {
        const std::size_t current = calcDisplayWidth(text);
        if (current >= totalWidth) return text;
        return text + std::string(totalWidth - current, ' ');
    }

    // ----------- 标题 / 提示 -----------
    inline void printSectionHeader(const std::string& title) {
        const std::string border(title.size() + 8, '=');
        applyColor(Color::Primary);
        std::cout << "\n" << border << "\n";
        std::cout << "==  " << title << "  ==\n";
        std::cout << border << "\n";
        resetColor();
    }

    inline void printSuccess(const std::string& message) {
        applyColor(Color::Success);
        std::cout << "[成功] " << message << '\n';
        resetColor();
    }

    inline void printInfo(const std::string& message) {
        applyColor(Color::Muted);
        std::cout << "[提示] " << message << '\n';
        resetColor();
    }

    inline void printWarning(const std::string& message) {
        applyColor(Color::Warning);
        std::cout << "[注意] " << message << '\n';
        resetColor();
    }

    inline void printError(const std::string& message) {
        applyColor(Color::Danger);
        std::cout << "[错误] " << message << '\n';
        resetColor();
    }

    // ----------- 通用表格渲染 -----------
    inline void printTable(const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows) {
        if (headers.empty()) return;
        const std::size_t cols = headers.size();
        std::vector<std::size_t> widths(cols);

        for (std::size_t i = 0; i < cols; ++i)
            widths[i] = std::max<std::size_t>(calcDisplayWidth(headers[i]), 4);

        for (const auto& row : rows) {
            for (std::size_t i = 0; i < cols && i < row.size(); ++i)
                widths[i] = std::max(widths[i], calcDisplayWidth(row[i]));
        }

        auto drawLine = [&](char left, char mid, char right) {
            std::cout << left;
            for (std::size_t i = 0; i < cols; ++i) {
                std::cout << std::string(widths[i] + 2, '-');
                std::cout << (i + 1 == cols ? right : mid);
            }
            std::cout << '\n';
            };

        drawLine('+', '+', '+');
        std::cout << '|';
        for (std::size_t i = 0; i < cols; ++i)
            std::cout << ' ' << padRight(headers[i], widths[i]) << ' ' << '|';
        std::cout << '\n';
        drawLine('+', '+', '+');

        for (const auto& row : rows) {
            std::cout << '|';
            for (std::size_t i = 0; i < cols; ++i) {
                const std::string cell = (i < row.size() ? row[i] : "");
                std::cout << ' ' << padRight(cell, widths[i]) << ' ' << '|';
            }
            std::cout << '\n';
        }
        drawLine('+', '+', '+');
    }

} // namespace ConsoleUI
