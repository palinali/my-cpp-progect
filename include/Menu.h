#pragma once
#ifndef PHARMACY_MENU_H
#define PHARMACY_MENU_H

#include "Medicine.h"
#include "Order.h"
#include "Repository.h"
#include "User.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace PharmacySystem {

    /**
     * @brief Главный контроллер консольного пользовательского интерфейса и бизнес-логики АИС.
     * Управляет авторизацией, маршрутизацией ролей (Администратор / Фармацевт),
     * CRUD-операциями над каталогом, поиском, сортировкой, фильтрацией и оформлением заказов.
     */
    class Menu {
    private:
        Storage::Repository<std::shared_ptr<Medicine>> catalog_;
        Storage::Repository<User> users_;
        Storage::Repository<Order> orders_;
        std::optional<User> currentUser_;

        // Внутренняя инициализация и начальное наполнение при первом запуске
        void seedDefaultUsersIfEmpty();
        void seedDefaultCatalogIfEmpty();
        void persistAllData();

        // Проверка прав доступа
        void requireAdminAccess() const;

        // Меню ролей
        void runAdminMenu();
        void runPharmacistMenu();

        // Отображение таблиц
        void printMedicinesTable(const std::vector<std::shared_ptr<Medicine>>& list,
            const std::string& tableTitle) const;
        void viewAllMedicines() const;

        // CRUD-операции (Администратор)
        void addMedicineInteractive();
        void editMedicineInteractive();
        void deleteMedicineInteractive();
        void restockMedicineInteractive();

        // Поиск, сортировка и фильтрация (перегруженные методы поиска)
        std::vector<std::shared_ptr<Medicine>> searchMedicines(const std::string& query) const;
        std::vector<std::shared_ptr<Medicine>> searchMedicines(double minPrice, double maxPrice) const;
        void handleSearchMenu() const;
        void handleSortMenu();
        void handleFilterMenu() const;

        // Оформление заказа / отпуска препаратов и работа с чеками
        void createOrderInteractive();
        void viewOrderHistory() const;

        // Управление пользователями (Администратор)
        void manageUsersMenu();
        void viewUsersList() const;
        void registerNewUserInteractive();
        void changeUserPasswordInteractive();
        void deleteUserInteractive();

        // Аналитика, сводные отчёты и резервное копирование
        void generateSummaryReportInteractive() const;
        void manageBackupMenu();

    public:
        Menu();
        ~Menu() = default;

        // Загрузка баз данных с диска при старте
        void initialize();

        // Главный цикл приложения (авторизация -> рабочая сессия -> выход)
        void run();
    };

} // namespace PharmacySystem

#endif // PHARMACY_MENU_H