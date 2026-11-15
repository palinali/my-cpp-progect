#pragma once
#ifndef PHARMACY_ORDER_H
#define PHARMACY_ORDER_H

#include "Medicine.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace PharmacySystem {

    /**
     * @brief Позиция в чеке / заказе покупателя.
     */
    struct OrderItem {
        int medicineId;
        std::string medicineName;
        std::string categoryName;
        int quantity;
        double unitBasePrice;
        double lineTotalPrice; // Итоговая стоимость позиции (с учётом надбавки ПКУ, если применимо)
        bool requiresRx;
    };

    /**
     * @brief Класс заказа (чека на отпуск лекарственных средств).
     * Поддерживает проверку остатков, перегрузку операторов (+=, <<) и генерацию текстовых отчётов.
     */
    class Order {
    private:
        int orderId_;
        std::string timestamp_;
        std::string pharmacistUsername_;
        std::string prescriptionDetails_; // Серия/номер рецепта (если в заказе есть рецептурные/ПКУ препараты)
        std::vector<OrderItem> items_;
        double totalAmount_;
        bool isFinalized_;

        // Статические поля для нумерации заказов и подсчёта общей выручки за сессию/БД
        static int nextOrderId_;
        static double totalAccumulatedRevenue_;

    public:
        explicit Order(std::string pharmacistUsername = "system");

        // Конструктор для восстановления заказа из файла истории
        Order(int orderId,
            std::string timestamp,
            std::string pharmacistUsername,
            std::string prescriptionDetails,
            double totalAmount,
            std::vector<OrderItem> items);

        // Добавление препарата в корзину с проверкой остатка и срока годности
        void addMedicineItem(const std::shared_ptr<Medicine>& medicine, int quantity);

        // Перегрузка оператора += для добавления готовой позиции OrderItem в заказ
        Order& operator+=(const OrderItem& item);

        // Проверка, требуется ли рецепт хотя бы для одной позиции в корзине
        bool hasPrescriptionItems() const noexcept;

        // Установка реквизитов рецепта
        void setPrescriptionDetails(const std::string& details);

        // Проведение заказа: списание остатков со склада (catalog) и фиксация выручки
        void finalizeOrder(std::vector<std::shared_ptr<Medicine>>& catalog);

        // Формирование структурированного кассового чека / накладной
        std::string generateReceiptText() const;

        // Сохранение чека в отдельный текстовый файл (.txt)
        bool exportReceiptToTxt(const std::string& filePath) const;

        // Геттеры
        inline int getOrderId() const noexcept { return orderId_; }
        inline const std::string& getTimestamp() const noexcept { return timestamp_; }
        inline const std::string& getPharmacistUsername() const noexcept { return pharmacistUsername_; }
        inline const std::string& getPrescriptionDetails() const noexcept { return prescriptionDetails_; }
        inline const std::vector<OrderItem>& getItems() const noexcept { return items_; }
        inline double getTotalAmount() const noexcept { return totalAmount_; }
        inline bool isFinalized() const noexcept { return isFinalized_; }
        inline bool isEmpty() const noexcept { return items_.empty(); }

        // Статические методы управления счётчиками
        static void syncNextOrderId(int maxExistingId) noexcept;
        static void addToTotalRevenue(double amount) noexcept;
        static void resetTotalRevenue() noexcept;
        static double getTotalAccumulatedRevenue() noexcept;

        // Сериализация и десериализация для хранения в файле истории заказов
        std::string serialize() const;
        static Order deserialize(const std::string& line);

        // Дружественный оператор вывода чека в поток (консоль или файл)
        friend std::ostream& operator<<(std::ostream& os, const Order& order);
    };

} // namespace PharmacySystem

#endif // PHARMACY_ORDER_H