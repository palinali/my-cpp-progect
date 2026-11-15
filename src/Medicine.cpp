#include "Medicine.h"
#include "ConsoleUtils.h"
#include "Exceptions.h"
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

namespace PharmacySystem {

    Medicine::Medicine(int id,
        std::string name,
        std::string activeIngredient,
        std::string manufacturer,
        double price,
        int stockQuantity,
        std::string expiryDate)
        : MedicalEntity(id, std::move(name)),
        activeIngredient_(std::move(activeIngredient)),
        manufacturer_(std::move(manufacturer)),
        price_(price),
        stockQuantity_(stockQuantity),
        expiryDate_(std::move(expiryDate)) {
        if (activeIngredient_.empty()) {
            throw Exceptions::ValidationException("Действующее вещество (МНН)", "Поле не может быть пустым.");
        }
        if (manufacturer_.empty()) {
            throw Exceptions::ValidationException("Производитель", "Поле не может быть пустым.");
        }
        if (price_ <= 0.0) {
            throw Exceptions::ValidationException("Цена", "Цена должна быть строго больше 0.");
        }
        if (stockQuantity_ < 0) {
            throw Exceptions::ValidationException("Количество", "Остаток на складе не может быть отрицательным.");
        }
        if (!Utils::ConsoleUtils::isValidDate(expiryDate_)) {
            throw Exceptions::ValidationException("Срок годности", "Ожидается корректная дата в формате ГГГГ-ММ-ДД.");
        }
    }

    void Medicine::setActiveIngredient(const std::string& activeIngredient) {
        if (activeIngredient.empty()) {
            throw Exceptions::ValidationException("Действующее вещество", "Поле не может быть пустым.");
        }
        activeIngredient_ = activeIngredient;
    }

    void Medicine::setManufacturer(const std::string& manufacturer) {
        if (manufacturer.empty()) {
            throw Exceptions::ValidationException("Производитель", "Поле не может быть пустым.");
        }
        manufacturer_ = manufacturer;
    }

    void Medicine::setPrice(double price) {
        if (price <= 0.0) {
            throw Exceptions::ValidationException("Цена", "Цена должна быть строго больше 0.");
        }
        price_ = price;
    }

    void Medicine::setStockQuantity(int quantity) {
        if (quantity < 0) {
            throw Exceptions::ValidationException("Количество", "Остаток не может быть отрицательным.");
        }
        stockQuantity_ = quantity;
    }

    void Medicine::setExpiryDate(const std::string& expiryDate) {
        if (!Utils::ConsoleUtils::isValidDate(expiryDate)) {
            throw Exceptions::ValidationException("Срок годности", "Ожидается дата в формате ГГГГ-ММ-ДД.");
        }
        expiryDate_ = expiryDate;
    }

    void Medicine::addStock(int amount) {
        if (amount <= 0) {
            throw Exceptions::ValidationException("Пополнение склада", "Количество для добавления должно быть > 0.");
        }
        stockQuantity_ += amount;
    }

    void Medicine::reduceStock(int amount) {
        if (amount <= 0) {
            throw Exceptions::ValidationException("Списание со склада", "Количество для списания должно быть > 0.");
        }
        if (isExpired()) {
            throw Exceptions::OutOfStockException("Препарат '" + name_ + "' (ID: " + std::to_string(id_) +
                ") просрочен (годен до " + expiryDate_ + "). Отпуск запрещён!");
        }
        if (amount > stockQuantity_) {
            throw Exceptions::OutOfStockException(name_, amount, stockQuantity_);
        }
        stockQuantity_ -= amount;
    }

    bool Medicine::isExpired() const {
        return Utils::ConsoleUtils::isDateExpired(expiryDate_);
    }

    bool Medicine::isLowStock(int threshold) const noexcept {
        return stockQuantity_ > 0 && stockQuantity_ <= threshold;
    }

    double Medicine::calculateFinalPrice(int quantity) const {
        if (quantity <= 0) {
            throw Exceptions::ValidationException("Количество в заказе", "Количество должно быть больше 0.");
        }
        return price_ * static_cast<double>(quantity);
    }

    std::string Medicine::serialize() const {
        std::ostringstream oss;
        oss << typeToCode(getType()) << "|"
            << id_ << "|"
            << name_ << "|"
            << activeIngredient_ << "|"
            << manufacturer_ << "|"
            << std::fixed << std::setprecision(2) << price_ << "|"
            << stockQuantity_ << "|"
            << expiryDate_;
        return oss.str();
    }

    void Medicine::printDetails(std::ostream& os) const {
        os << "ID: " << id_ << " | Наименование: " << name_ << "\n"
            << "  МНН (действ. вещество): " << activeIngredient_ << "\n"
            << "  Категория:              " << getCategoryName() << "\n"
            << "  Производитель:          " << manufacturer_ << "\n"
            << "  Цена за ед.:            " << std::fixed << std::setprecision(2) << price_ << " руб.\n"
            << "  Остаток на складе:      " << stockQuantity_ << " уп.\n"
            << "  Срок годности:          " << expiryDate_
            << (isExpired() ? " [ПРОСРОЧЕН!]" : " [Годен]") << "\n"
            << "  Условия отпуска:        " << getDispenseRule() << "\n";
    }

    void Medicine::printTableRow(std::ostream& os) const {
        std::ostringstream priceStream;
        priceStream << std::fixed << std::setprecision(2) << price_;

        std::string stockStr = std::to_string(stockQuantity_);
        if (stockQuantity_ == 0) {
            stockStr += " (нет)";
        }
        else if (isLowStock()) {
            stockStr += " (!)";
        }

        std::string expStr = expiryDate_ + (isExpired() ? "*" : "");
        std::string rxStr = requiresPrescription() ? "Да" : "Нет";

        os << "| " << Utils::ConsoleUtils::padRightUTF8(std::to_string(id_), 4)
            << " | " << Utils::ConsoleUtils::padRightUTF8(name_, 20)
            << " | " << Utils::ConsoleUtils::padRightUTF8(activeIngredient_, 18)
            << " | " << Utils::ConsoleUtils::padRightUTF8(getCategoryName(), 16)
            << " | " << Utils::ConsoleUtils::padRightUTF8(manufacturer_, 15)
            << " | " << Utils::ConsoleUtils::padRightUTF8(priceStream.str(), 9)
            << " | " << Utils::ConsoleUtils::padRightUTF8(stockStr, 10)
            << " | " << Utils::ConsoleUtils::padRightUTF8(expStr, 11)
            << " | " << Utils::ConsoleUtils::padRightUTF8(rxStr, 6)
            << " |\n";
    }

    std::string Medicine::typeToCode(MedicineType type) {
        switch (type) {
        case MedicineType::OTC:
            return "OTC";
        case MedicineType::PRESCRIPTION:
            return "RX";
        case MedicineType::NARCOTIC_PKU:
            return "PKU";
        default:
            return "OTC";
        }
    }

    MedicineType Medicine::codeToType(const std::string& code) {
        if (code == "OTC") return MedicineType::OTC;
        if (code == "RX") return MedicineType::PRESCRIPTION;
        if (code == "PKU") return MedicineType::NARCOTIC_PKU;
        throw Exceptions::ValidationException("Тип препарата", "Неизвестный код категории: " + code);
    }

    bool Medicine::operator==(const Medicine& other) const noexcept {
        return id_ == other.id_;
    }

    bool Medicine::operator<(const Medicine& other) const noexcept {
        return Utils::ConsoleUtils::toLowerUTF8(name_) < Utils::ConsoleUtils::toLowerUTF8(other.name_);
    }

    std::ostream& operator<<(std::ostream& os, const Medicine& med) {
        os << med.serialize();
        return os;
    }

    std::istream& operator>>(std::istream& is, Medicine& med) {
        std::string line;
        if (!std::getline(is, line)) {
            return is;
        }

        line = Utils::ConsoleUtils::trim(line);
        if (line.empty()) {
            is.setstate(std::ios::failbit);
            return is;
        }

        std::vector<std::string> parts = Utils::ConsoleUtils::split(line, '|');
        if (parts.size() < 8) {
            is.setstate(std::ios::failbit);
            return is;
        }

        try {
            int parsedId = std::stoi(parts[1]);
            std::string parsedName = parts[2];
            std::string parsedMnn = parts[3];
            std::string parsedManuf = parts[4];
            double parsedPrice = std::stod(parts[5]);
            int parsedStock = std::stoi(parts[6]);
            std::string parsedExpiry = parts[7];

            med.id_ = parsedId;
            med.setName(parsedName);
            med.setActiveIngredient(parsedMnn);
            med.setManufacturer(parsedManuf);
            med.setPrice(parsedPrice);
            med.setStockQuantity(parsedStock);
            med.setExpiryDate(parsedExpiry);
            MedicalEntity::syncNextId(parsedId);
        }
        catch (...) {
            is.setstate(std::ios::failbit);
        }

        return is;
    }

} // namespace PharmacySystem