#pragma once
#ifndef PHARMACY_DERIVED_MEDICINES_H
#define PHARMACY_DERIVED_MEDICINES_H

#include "Medicine.h"
#include <string>

namespace PharmacySystem {

    /**
     * @brief Уровень 3 иерархии (ветвь 1): Безрецептурное лекарственное средство (OTC).
     * Отпускается свободно без предъявления рецептурного бланка.
     */
    class OTCMedicine : public Medicine {
    private:
        std::string pharmacologicalGroup_; // Фармакотерапевтическая группа (например, "Анальгетики")

    public:
        OTCMedicine(int id,
            std::string name,
            std::string activeIngredient,
            std::string manufacturer,
            double price,
            int stockQuantity,
            std::string expiryDate,
            std::string pharmacologicalGroup = "Общая терапия");

        ~OTCMedicine() noexcept override = default;

        inline const std::string& getPharmacologicalGroup() const noexcept { return pharmacologicalGroup_; }
        void setPharmacologicalGroup(const std::string& group);

        // Переопределение виртуальных методов
        MedicineType getType() const noexcept override { return MedicineType::OTC; }
        bool requiresPrescription() const noexcept override { return false; }
        std::string getCategoryName() const override;
        std::string getDispenseRule() const override;
        std::string serialize() const override;
        void printDetails(std::ostream& os) const override;
    };

    /**
     * @brief Уровень 3 иерархии (ветвь 2): Рецептурное лекарственное средство (Rx).
     * Требует обязательного ввода серии и номера рецептурного бланка при оформлении заказа.
     */
    class PrescriptionMedicine : public Medicine {
    private:
        std::string prescriptionFormType_; // Тип бланка (например, "Форма 107-1/у")

    public:
        PrescriptionMedicine(int id,
            std::string name,
            std::string activeIngredient,
            std::string manufacturer,
            double price,
            int stockQuantity,
            std::string expiryDate,
            std::string prescriptionFormType = "Форма 107-1/у");

        ~PrescriptionMedicine() noexcept override = default;

        inline const std::string& getPrescriptionFormType() const noexcept { return prescriptionFormType_; }
        void setPrescriptionFormType(const std::string& formType);

        // Переопределение виртуальных методов
        MedicineType getType() const noexcept override { return MedicineType::PRESCRIPTION; }
        bool requiresPrescription() const noexcept override { return true; }
        std::string getCategoryName() const override;
        std::string getDispenseRule() const override;
        std::string serialize() const override;
        void printDetails(std::ostream& os) const override;
    };

    /**
     * @brief Уровень 3 иерархии (ветвь 3): Препарат предметно-количественного учёта (ПКУ).
     * Строгий контроль отпуска (Форма 148-1/у-88), специальный код лицензии и надбавка за спецучёт/хранение.
     */
    class NarcoticControlMedicine : public Medicine {
    private:
        std::string registryLicenseCode_; // Код спецучёта / сейфового хранения
        double specialHandlingFeePercent_; // Надбавка за спецучёт и особое хранение (%)

    public:
        NarcoticControlMedicine(int id,
            std::string name,
            std::string activeIngredient,
            std::string manufacturer,
            double price,
            int stockQuantity,
            std::string expiryDate,
            std::string registryLicenseCode = "ПКУ-148/у-88",
            double specialHandlingFeePercent = 5.0);

        ~NarcoticControlMedicine() noexcept override = default;

        inline const std::string& getRegistryLicenseCode() const noexcept { return registryLicenseCode_; }
        inline double getSpecialHandlingFeePercent() const noexcept { return specialHandlingFeePercent_; }
        void setRegistryLicenseCode(const std::string& code);
        void setSpecialHandlingFeePercent(double percent);

        // Переопределение расчёта итоговой стоимости с учётом надбавки за спецучёт
        double calculateFinalPrice(int quantity) const override;

        // Переопределение виртуальных методов
        MedicineType getType() const noexcept override { return MedicineType::NARCOTIC_PKU; }
        bool requiresPrescription() const noexcept override { return true; }
        std::string getCategoryName() const override;
        std::string getDispenseRule() const override;
        std::string serialize() const override;
        void printDetails(std::ostream& os) const override;
    };

} // namespace PharmacySystem

#endif // PHARMACY_DERIVED_MEDICINES_H