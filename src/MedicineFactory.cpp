#include "MedicineFactory.h"
#include "ConsoleUtils.h"
#include "Exceptions.h"
#include <vector>

namespace PharmacySystem {

    std::shared_ptr<Medicine> MedicineFactory::createMedicine(MedicineType type,
        int id,
        const std::string& name,
        const std::string& activeIngredient,
        const std::string& manufacturer,
        double price,
        int stockQuantity,
        const std::string& expiryDate,
        const std::string& extraParam1,
        double extraParam2) {
        int assignedId = (id > 0) ? id : MedicalEntity::generateNextId();

        switch (type) {
        case MedicineType::OTC:
            return std::make_shared<OTCMedicine>(
                assignedId,
                name,
                activeIngredient,
                manufacturer,
                price,
                stockQuantity,
                expiryDate,
                extraParam1.empty() ? "Общая терапия" : extraParam1
            );

        case MedicineType::PRESCRIPTION:
            return std::make_shared<PrescriptionMedicine>(
                assignedId,
                name,
                activeIngredient,
                manufacturer,
                price,
                stockQuantity,
                expiryDate,
                extraParam1.empty() ? "Форма 107-1/у" : extraParam1
            );

        case MedicineType::NARCOTIC_PKU:
            return std::make_shared<NarcoticControlMedicine>(
                assignedId,
                name,
                activeIngredient,
                manufacturer,
                price,
                stockQuantity,
                expiryDate,
                extraParam1.empty() ? "ПКУ-148/у-88" : extraParam1,
                extraParam2
            );

        default:
            throw Exceptions::ValidationException("Фабрика препаратов", "Неподдерживаемый тип лекарственного средства.");
        }
    }

    std::shared_ptr<Medicine> MedicineFactory::createFromSerializedString(const std::string& serializedLine) {
        std::string trimmedLine = Utils::ConsoleUtils::trim(serializedLine);
        if (trimmedLine.empty()) {
            throw Exceptions::ValidationException("Десериализация препарата", "Пустая строка записи.");
        }

        std::vector<std::string> parts = Utils::ConsoleUtils::split(trimmedLine, '|');
        if (parts.size() < 8) {
            throw Exceptions::ValidationException("Десериализация препарата",
                "Некорректное число полей в строке: " + trimmedLine);
        }

        MedicineType type = Medicine::codeToType(parts[0]);
        int id = 0;
        double price = 0.0;
        int stock = 0;

        try {
            id = std::stoi(parts[1]);
            price = std::stod(parts[5]);
            stock = std::stoi(parts[6]);
        }
        catch (const std::exception&) {
            throw Exceptions::ValidationException("Десериализация препарата",
                "Ошибка парсинга числовых полей (ID/Цена/Остаток) в строке: " + trimmedLine);
        }

        const std::string& name = parts[2];
        const std::string& mnn = parts[3];
        const std::string& manufacturer = parts[4];
        const std::string& expiry = parts[7];

        std::string extra1 = (parts.size() >= 9) ? parts[8] : "";
        double extra2 = 5.0;
        if (parts.size() >= 10 && !parts[9].empty()) {
            try {
                extra2 = std::stod(parts[9]);
            }
            catch (const std::exception&) {
                throw Exceptions::ValidationException("Десериализация препарата",
                    "Ошибка парсинга надбавки ПКУ в строке: " + trimmedLine);
            }
        }

        return createMedicine(type, id, name, mnn, manufacturer, price, stock, expiry, extra1, extra2);
    }

} // namespace PharmacySystem