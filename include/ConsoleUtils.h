#pragma once
#ifndef PHARMACY_CONSOLE_UTILS_H
#define PHARMACY_CONSOLE_UTILS_H

#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace Utils {

    /**
     * @brief Утилитарный класс для кроссплатформенной работы с консолью (UTF-8),
     *        безопасного потокового ввода с проверкой ошибок и обработки строк/дат.
     */
    class ConsoleUtils {
    public:
        // Настройка кодировки UTF-8 и локали для Windows 10+ и macOS/Linux
        static void initConsoleUTF8();

        // Очистка экрана и пауза
        static void clearScreen();
        static void pause(const std::string& message = "\nНажмите Enter для продолжения...");

        // Вспомогательные методы форматирования вывода
        static void printHeader(const std::string& title, int width = 100);
        static void printSeparator(char ch = '-', int width = 100);

        // Вычисление визуальной ширины строки в символах UTF-8 (критично для ровных таблиц с кириллицей)
        static size_t utf8Length(const std::string& str);
        static std::string padRightUTF8(const std::string& str, size_t width);
        static std::string truncateUTF8(const std::string& str, size_t maxWidth);

        // Обработка строк и валидация дат (ГГГГ-ММ-ДД)
        static std::string trim(const std::string& str);
        static std::string toLowerUTF8(const std::string& str);
        static std::vector<std::string> split(const std::string& str, char delimiter);
        static bool isValidDate(const std::string& dateStr);
        static bool isDateExpired(const std::string& expiryDateStr);
        static std::string getCurrentDate();
        static std::string getCurrentDateTime();

        // Безопасный ввод строки с проверкой состояния потока std::cin
        static std::string readLine(const std::string& prompt, bool allowEmpty = false);
        static bool readConfirmation(const std::string& prompt);

        /**
         * @brief Шаблонный метод для безопасного чтения чисел с клавиатуры
         *        с проверкой cin.fail(), очисткой буфера cin.ignore() и проверкой диапазона.
         */
        template <typename T>
        static T readNumber(const std::string& prompt,
            T minVal = std::numeric_limits<T>::lowest(),
            T maxVal = std::numeric_limits<T>::max()) {
            T value{};
            while (true) {
                std::cout << prompt;
                if (!(std::cin >> value)) {
                    if (std::cin.eof()) {
                        std::cin.clear();
                        throw std::runtime_error("Поток ввода неожиданно прерван (EOF).");
                    }
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "  [Ошибка ввода] Пожалуйста, введите корректное число.\n";
                    continue;
                }

                // Проверяем, не осталось ли мусора в строке после числа (например, "12abc")
                char nextChar = static_cast<char>(std::cin.peek());
                if (nextChar != '\n' && nextChar != EOF && nextChar != ' ' && nextChar != '\t') {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "  [Ошибка ввода] Обнаружены недопустимые символы после числа.\n";
                    continue;
                }

                // Очищаем остаток буфера до символа новой строки
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                if (value < minVal || value > maxVal) {
                    std::cout << "  [Ошибка диапазона] Значение должно быть в пределах от "
                        << minVal << " до " << maxVal << ".\n";
                    continue;
                }

                return value;
            }
        }
    };

} // namespace Utils

#endif // PHARMACY_CONSOLE_UTILS_H