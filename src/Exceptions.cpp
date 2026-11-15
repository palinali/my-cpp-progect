#include "Exceptions.h"
#include <utility>

namespace Exceptions {

    PharmacyException::PharmacyException(std::string message)
        : message_(std::move(message)) {
    }

    const char* PharmacyException::what() const noexcept {
        return message_.c_str();
    }

    FileAccessException::FileAccessException(const std::string& filePath, const std::string& details)
        : PharmacyException("Ошибка файловой системы при работе с '" + filePath + "'" +
            (details.empty() ? "." : ": " + details)) {
    }

    ValidationException::ValidationException(const std::string& fieldOrContext, const std::string& details)
        : PharmacyException("Ошибка валидации [" + fieldOrContext + "]: " + details) {
    }

    OutOfStockException::OutOfStockException(const std::string& medicineName, int requested, int available)
        : PharmacyException("Недостаточно препарата '" + medicineName + "' на складе (запрошено: " +
            std::to_string(requested) + ", доступно: " + std::to_string(available) + ").") {
    }

    OutOfStockException::OutOfStockException(const std::string& details)
        : PharmacyException("Ошибка отпуска товара: " + details) {
    }

    AuthException::AuthException(const std::string& details)
        : PharmacyException("Ошибка авторизации / доступа: " + details) {
    }

} // namespace Exceptions