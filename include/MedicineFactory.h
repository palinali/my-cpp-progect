#pragma once
#ifndef PHARMACY_MEDICINE_FACTORY_H
#define PHARMACY_MEDICINE_FACTORY_H

#include "DerivedMedicines.h"
#include <memory>
#include <string>

namespace PharmacySystem {

    /**
     * @brief Паттерн проектирования Factory Method (Фабричный метод).
     * Инкапсулирует логику создания полиморфных объектов лекарственных средств
     * различных категорий (OTC, Рецептурные, ПКУ) при вводе из консоли или загрузке из файла.
     */
    class MedicineFactory {
    public:
        /**
         * @brief Создаёт объект лекарственного средства нужного подкласса по типу.
         * @param type Тип препарата (OTC, PRESCRIPTION, NARCOTIC_PKU).
         * @param id Уникальный идентификатор (если <= 0, генерируется автоматически).
         * @param name Торговое наименование.
         * @param activeIngredient Действующее вещество (МНН).
         * @param manufacturer Производитель.
         * @param price Розничная цена за упаковку.
         * @param stockQuantity Количество упаковок на складе.
         * @param expiryDate Срок годности (ГГГГ-ММ-ДД).
         * @param extraParam1 Доп. параметр (фарм. группа / форма рецепта / код ПКУ).
         * @param extraParam2 Доп. числовой параметр (процент надбавки для ПКУ).
         * @return Умный указатель std::shared_ptr<Medicine> на созданный объект.
         */
        static std::shared_ptr<Medicine> createMedicine(MedicineType type,
            int id,
            const std::string& name,
            const std::string& activeIngredient,
            const std::string& manufacturer,
            double price,
            int stockQuantity,
            const std::string& expiryDate,
            const std::string& extraParam1 = "",
            double extraParam2 = 5.0);

        /**
         * @brief Восстанавливает полиморфный объект препарата из строки текстовой БД.
         * @param serializedLine Строка формата TYPE|ID|NAME|MNN|MANUF|PRICE|STOCK|EXPIRY|EXTRA...
         */
        static std::shared_ptr<Medicine> createFromSerializedString(const std::string& serializedLine);
    };

} // namespace PharmacySystem

#endif // PHARMACY_MEDICINE_FACTORY_H