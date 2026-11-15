#pragma once
#ifndef PHARMACY_EXCEPTIONS_H
#define PHARMACY_EXCEPTIONS_H

#include <exception>
#include <string>

namespace Exceptions {

    /**
     * @brief Базовый класс исключений информационной системы аптеки.
     * Наследуется от стандартного std::exception.
     */
    class PharmacyException : public std::exception {
    protected:
        std::string message_;

    public:
        explicit PharmacyException(std::string message);
        virtual ~PharmacyException() noexcept override = default;

        const char* what() const noexcept override;
    };

    /**
     * @brief Исключение при ошибках открытия, чтения или записи файлов БД и отчётов.
     */
    class FileAccessException : public PharmacyException {
    public:
        explicit FileAccessException(const std::string& filePath, const std::string& details = "");
    };

    /**
     * @brief Исключение при нарушении правил валидации данных (отрицательная цена, неверная дата и т.д.).
     */
    class ValidationException : public PharmacyException {
    public:
        explicit ValidationException(const std::string& fieldOrContext, const std::string& details);
    };

    /**
     * @brief Исключение при нехватке лекарственного средства на складе или попытке отпуска просроченного препарата.
     */
    class OutOfStockException : public PharmacyException {
    public:
        OutOfStockException(const std::string& medicineName, int requested, int available);
        explicit OutOfStockException(const std::string& details);
    };

    /**
     * @brief Исключение при ошибках аутентификации или нарушении прав доступа роли.
     */
    class AuthException : public PharmacyException {
    public:
        explicit AuthException(const std::string& details);
    };

} // namespace Exceptions

#endif // PHARMACY_EXCEPTIONS_H