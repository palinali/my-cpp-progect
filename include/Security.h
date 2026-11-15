#pragma once
#ifndef PHARMACY_SECURITY_H
#define PHARMACY_SECURITY_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace Security {

    /**
     * @brief Динамический криптографический буфер с ручным управлением памятью (new[] / delete[]).
     * Реализует Правило Пяти (деструктор, конструктор копирования, оператор присваивания
     * копированием, конструктор перемещения, оператор присваивания перемещением).
     */
    class CryptoBuffer {
    private:
        uint8_t* data_;
        size_t size_;

    public:
        // Конструкторы
        explicit CryptoBuffer(size_t size = 0);
        explicit CryptoBuffer(const std::string& text);

        // 1. Деструктор
        ~CryptoBuffer();

        // 2. Конструктор копирования (глубокое копирование динамического массива)
        CryptoBuffer(const CryptoBuffer& other);

        // 3. Оператор присваивания копированием (с защитой от самоприсваивания)
        CryptoBuffer& operator=(const CryptoBuffer& other);

        // 4. Конструктор перемещения
        CryptoBuffer(CryptoBuffer&& other) noexcept;

        // 5. Оператор присваивания перемещением
        CryptoBuffer& operator=(CryptoBuffer&& other) noexcept;

        // Криптографические преобразования над буфером
        void applyXorWithRotation(uint8_t baseKey);
        std::string toHexString() const;

        inline size_t size() const noexcept { return size_; }
        inline const uint8_t* rawData() const noexcept { return data_; }
    };

    /**
     * @brief Сервис хеширования паролей и генерации соли.
     */
    class PasswordHasher {
    public:
        static std::string generateSalt(size_t length = 16);
        static std::string hashPassword(const std::string& password, const std::string& salt);
        static bool verifyPassword(const std::string& password,
            const std::string& salt,
            const std::string& expectedHash);
    };

} // namespace Security

#endif // PHARMACY_SECURITY_H