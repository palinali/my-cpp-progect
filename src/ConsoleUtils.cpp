#include "ConsoleUtils.h"
#include <algorithm>
#include <chrono>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Utils {

    void ConsoleUtils::initConsoleUTF8() {
#ifdef _WIN32
        // Устанавливаем кодовую страницу UTF-8 (65001) для ввода и вывода консоли Windows 10+
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);

        // Включаем поддержку виртуального терминала (ANSI escape-последовательности)
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
        std::setlocale(LC_ALL, ".UTF-8");
#else
        // Для macOS 12+ и Linux используем системную UTF-8 локаль
        if (!std::setlocale(LC_ALL, "ru_RU.UTF-8")) {
            std::setlocale(LC_ALL, "en_US.UTF-8");
        }
#endif
    }

    void ConsoleUtils::clearScreen() {
#ifdef _WIN32
        std::system("cls");
#else
        std::cout << "\033[2J\033[1;1H";
#endif
    }

    void ConsoleUtils::pause(const std::string& message) {
        std::cout << message;
        std::cin.clear();
        if (std::cin.rdbuf()->in_avail() > 0) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::cin.get();
    }

    void ConsoleUtils::printHeader(const std::string& title, int width) {
        printSeparator('=', width);
        size_t titleLen = utf8Length(title);
        int padding = (width > static_cast<int>(titleLen)) ? (width - static_cast<int>(titleLen)) / 2 : 0;
        std::cout << std::string(padding, ' ') << title << "\n";
        printSeparator('=', width);
    }

    void ConsoleUtils::printSeparator(char ch, int width) {
        std::cout << std::string(width, ch) << "\n";
    }

    size_t ConsoleUtils::utf8Length(const std::string& str) {
        size_t count = 0;
        for (unsigned char c : str) {
            // В UTF-8 байты продолжения имеют вид 10xxxxxx (0x80..0xBF)
            if ((c & 0xC0) != 0x80) {
                ++count;
            }
        }
        return count;
    }

    std::string ConsoleUtils::padRightUTF8(const std::string& str, size_t width) {
        std::string truncated = truncateUTF8(str, width);
        size_t len = utf8Length(truncated);
        if (len < width) {
            return truncated + std::string(width - len, ' ');
        }
        return truncated;
    }

    std::string ConsoleUtils::truncateUTF8(const std::string& str, size_t maxWidth) {
        if (utf8Length(str) <= maxWidth) {
            return str;
        }
        if (maxWidth <= 2) {
            return std::string(maxWidth, '.');
        }

        size_t targetChars = maxWidth - 2;
        size_t currentChars = 0;
        size_t byteIndex = 0;

        while (byteIndex < str.size() && currentChars < targetChars) {
            unsigned char c = static_cast<unsigned char>(str[byteIndex]);
            size_t charBytes = 1;
            if ((c & 0x80) == 0x00) charBytes = 1;
            else if ((c & 0xE0) == 0xC0) charBytes = 2;
            else if ((c & 0xF0) == 0xE0) charBytes = 3;
            else if ((c & 0xF8) == 0xF0) charBytes = 4;

            byteIndex += charBytes;
            ++currentChars;
        }

        return str.substr(0, byteIndex) + "..";
    }

    std::string ConsoleUtils::trim(const std::string& str) {
        const char* whitespace = " \t\n\r\f\v";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) {
            return "";
        }
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    std::string ConsoleUtils::toLowerUTF8(const std::string& str) {
        std::string result;
        result.reserve(str.size());

        for (size_t i = 0; i < str.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            // ASCII A-Z
            if (c >= 'A' && c <= 'Z') {
                result.push_back(static_cast<char>(c + 32));
            }
            // Кириллица UTF-8 (2 байта)
            else if (i + 1 < str.size()) {
                unsigned char c2 = static_cast<unsigned char>(str[i + 1]);
                // А-П: D0 90 .. D0 9F -> а-п: D0 B0 .. D0 BF
                if (c == 0xD0 && c2 >= 0x90 && c2 <= 0x9F) {
                    result.push_back(static_cast<char>(0xD0));
                    result.push_back(static_cast<char>(c2 + 0x20));
                    ++i;
                }
                // Р-Я: D0 A0 .. D0 AF -> р-я: D1 80 .. D1 8F
                else if (c == 0xD0 && c2 >= 0xA0 && c2 <= 0xAF) {
                    result.push_back(static_cast<char>(0xD1));
                    result.push_back(static_cast<char>(c2 - 0x20));
                    ++i;
                }
                // Ё: D0 81 -> ё: D1 91
                else if (c == 0xD0 && c2 == 0x81) {
                    result.push_back(static_cast<char>(0xD1));
                    result.push_back(static_cast<char>(0x91));
                    ++i;
                }
                else {
                    result.push_back(static_cast<char>(c));
                }
            }
            else {
                result.push_back(static_cast<char>(c));
            }
        }
        return result;
    }

    std::vector<std::string> ConsoleUtils::split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            tokens.push_back(item);
        }
        // Если строка заканчивается разделителем, добавляем пустой элемент
        if (!str.empty() && str.back() == delimiter) {
            tokens.emplace_back("");
        }
        return tokens;
    }

    bool ConsoleUtils::isValidDate(const std::string& dateStr) {
        if (dateStr.size() != 10 || dateStr[4] != '-' || dateStr[7] != '-') {
            return false;
        }
        for (size_t i = 0; i < 10; ++i) {
            if (i == 4 || i == 7) continue;
            if (dateStr[i] < '0' || dateStr[i] > '9') return false;
        }

        int year = std::stoi(dateStr.substr(0, 4));
        int month = std::stoi(dateStr.substr(5, 2));
        int day = std::stoi(dateStr.substr(8, 2));

        if (year < 2000 || year > 2100) return false;
        if (month < 1 || month > 12) return false;

        int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (isLeap) daysInMonth[2] = 29;

        return day >= 1 && day <= daysInMonth[month];
    }

    std::string ConsoleUtils::getCurrentDate() {
        auto now = std::chrono::system_clock::now();
        std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTm{};
#ifdef _WIN32
        localtime_s(&localTm, &nowTime);
#else
        localtime_r(&nowTime, &localTm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&localTm, "%Y-%m-%d");
        return oss.str();
    }

    std::string ConsoleUtils::getCurrentDateTime() {
        auto now = std::chrono::system_clock::now();
        std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTm{};
#ifdef _WIN32
        localtime_s(&localTm, &nowTime);
#else
        localtime_r(&nowTime, &localTm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    bool ConsoleUtils::isDateExpired(const std::string& expiryDateStr) {
        if (!isValidDate(expiryDateStr)) {
            return false;
        }
        // Формат ГГГГ-ММ-ДД позволяет лексикографическое сравнение дат
        return expiryDateStr < getCurrentDate();
    }

    std::string ConsoleUtils::readLine(const std::string& prompt, bool allowEmpty) {
        std::string input;
        while (true) {
            std::cout << prompt;
            if (!std::getline(std::cin, input)) {
                if (std::cin.eof()) {
                    std::cin.clear();
                    throw std::runtime_error("Поток ввода неожиданно завершён.");
                }
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "  [Ошибка ввода] Повторите попытку.\n";
                continue;
            }

            input = trim(input);
            if (!allowEmpty && input.empty()) {
                std::cout << "  [Ошибка ввода] Поле не может быть пустым.\n";
                continue;
            }

            // Запрещаем символ '|' в строках, так как он используется как разделитель в БД
            if (input.find('|') != std::string::npos) {
                std::cout << "  [Ошибка ввода] Символ '|' зарезервирован системой. Используйте другой символ.\n";
                continue;
            }

            return input;
        }
    }

    bool ConsoleUtils::readConfirmation(const std::string& prompt) {
        while (true) {
            std::string ans = toLowerUTF8(readLine(prompt + " (д/н): ", false));
            if (ans == "д" || ans == "да" || ans == "y" || ans == "yes") {
                return true;
            }
            if (ans == "н" || ans == "нет" || ans == "n" || ans == "no") {
                return false;
            }
            std::cout << "  [Ошибка ввода] Введите 'д' (да) или 'н' (нет).\n";
        }
    }

} // namespace Utils