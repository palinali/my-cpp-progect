#pragma once
#ifndef PHARMACY_MEDICAL_ENTITY_H
#define PHARMACY_MEDICAL_ENTITY_H

#include <iostream>
#include <string>

namespace PharmacySystem {

    /**
     * @brief Уровень 1 иерархии: Абстрактный базовый класс для всех сущностей медицинского каталога.
     * Содержит чисто виртуальные функции (= 0), виртуальный деструктор и статические счётчики.
     */
    class MedicalEntity {
    protected:
        int id_;
        std::string name_;

        // Статические поля для автоинкремента ID и отслеживания числа объектов в памяти
        static int nextId_;
        static int activeObjectCount_;

    public:
        MedicalEntity(int id, std::string name);
        virtual ~MedicalEntity() noexcept;

        // Встроенные (inline) геттеры и сеттеры
        inline int getId() const noexcept { return id_; }
        inline const std::string& getName() const noexcept { return name_; }
        void setName(const std::string& name);

        // Статические методы управления счётчиками
        static int generateNextId() noexcept;
        static void syncNextId(int maxExistingId) noexcept;
        static int getActiveObjectCount() noexcept;

        // Чисто виртуальные методы (делают класс абстрактным)
        virtual std::string getCategoryName() const = 0;
        virtual std::string getDispenseRule() const = 0;
        virtual std::string serialize() const = 0;
        virtual void printDetails(std::ostream& os) const = 0;
        virtual void printTableRow(std::ostream& os) const = 0;
    };

} // namespace PharmacySystem

#endif // PHARMACY_MEDICAL_ENTITY_H