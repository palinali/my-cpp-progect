#pragma once
#ifndef PHARMACY_MEDICINE_H
#define PHARMACY_MEDICINE_H

#include "MedicalEntity.h"
#include <iostream>
#include <string>

namespace PharmacySystem {

    /**
     * @brief Типы отпуска / категории лекарственных средств для фабрики и сериализации.
     */
    enum class MedicineType {
        OTC = 1,          // Безрецептурный препарат
        PRESCRIPTION = 2, // Рецептурный препарат
        NARCOTIC_PKU = 3  // Препарат предметно-количественного учёта (ПКУ)
    };

    /**
     * @brief Уровень 2 иерархии: Базовый класс лекарственного средства.
     * Инкапсулирует общие фармакологические и складские характеристики препарата.
     */
    class Medicine : public MedicalEntity {
    protected:
        std::string activeIngredient_; // МНН / Действующее вещество
        std::string manufacturer_;     // Производитель
        double price_;                 // Базовая розничная цена за упаковку (руб.)
        int stockQuantity_;            // Остаток на складе (упаковок)
        std::string expiryDate_;       // Срок годности в формате ГГГГ-ММ-ДД

    public:
        Medicine(int id,
            std::string name,
            std::string activeIngredient,
            std::string manufacturer,
            double price,
            int stockQuantity,
            std::string expiryDate);

        virtual ~Medicine() noexcept override = default;

        // Встроенные геттеры
        inline const std::string& getActiveIngredient() const noexcept { return activeIngredient_; }
        inline const std::string& getManufacturer() const noexcept { return manufacturer_; }
        inline double getPrice() const noexcept { return price_; }
        inline int getStockQuantity() const noexcept { return stockQuantity_; }
        inline const std::string& getExpiryDate() const noexcept { return expiryDate_; }

        // Сеттеры с валидацией
        void setActiveIngredient(const std::string& activeIngredient);
        void setManufacturer(const std::string& manufacturer);
        void setPrice(double price);
        void setStockQuantity(int quantity);
        void setExpiryDate(const std::string& expiryDate);

        // Складские операции
        void addStock(int amount);
        void reduceStock(int amount);
        bool isExpired() const;
        bool isLowStock(int threshold = 10) const noexcept;

        // Виртуальные методы, переопределяемые в классах 3-го уровня
        virtual MedicineType getType() const noexcept = 0;
        virtual bool requiresPrescription() const noexcept = 0;
        virtual double calculateFinalPrice(int quantity) const;

        // Переопределение методов MedicalEntity
        std::string serialize() const override;
        void printDetails(std::ostream& os) const override;
        void printTableRow(std::ostream& os) const override;

        // Статические преобразователи типа
        static std::string typeToCode(MedicineType type);
        static MedicineType codeToType(const std::string& code);

        // Перегрузка операторов сравнения (по ID и по алфавиту названия)
        bool operator==(const Medicine& other) const noexcept;
        bool operator<(const Medicine& other) const noexcept;

        // Дружественные операторы потокового ввода/вывода
        friend std::ostream& operator<<(std::ostream& os, const Medicine& med);
        friend std::istream& operator>>(std::istream& is, Medicine& med);
    };

} // namespace PharmacySystem

#endif // PHARMACY_MEDICINE_H