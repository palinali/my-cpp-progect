#pragma once
#ifndef PHARMACY_FILE_MANAGER_H
#define PHARMACY_FILE_MANAGER_H

#include "Medicine.h"
#include "Order.h"
#include "User.h"
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace PharmacySystem {

    /**
     * @brief Уровни логирования системных событий.
     */
    enum class LogLevel {
        INFO,
        WARNING,
        ERROR_LEVEL,
        AUDIT
    };

    /**
     * @brief Паттерн проектирования Singleton (Одиночка Мейерса).
     * Отвечает за централизованное файловое хранилище (БД лекарств, пользователей, истории заказов),
     * бинарный экспорт/бэкап каталога, генерацию сводных отчётов и ведение журнала аудита.
     */
    class FileManager {
    private:
        std::string dataDirectory_;
        std::string medicinesFilePath_;
        std::string usersFilePath_;
        std::string ordersFilePath_;
        std::string logFilePath_;
        std::string binaryBackupPath_;

        // Приватный конструктор и запрет копирования/перемещения (Singleton)
        FileManager();
        ~FileManager() = default;

        FileManager(const FileManager&) = delete;
        FileManager& operator=(const FileManager&) = delete;
        FileManager(FileManager&&) = delete;
        FileManager& operator=(FileManager&&) = delete;

        void ensureDirectoryExists() const;

    public:
        // Точка доступа к единственному экземпляру Singleton
        static FileManager& getInstance();

        // Журналирование действий в файл audit.log
        void logEvent(LogLevel level, const std::string& message);

        // Работа с базой пользователей (users.db)
        std::vector<User> loadUsers();
        void saveUsers(const std::vector<User>& users);

        // Работа с каталогом лекарственных средств (medicines.db)
        std::vector<std::shared_ptr<Medicine>> loadMedicines();
        void saveMedicines(const std::vector<std::shared_ptr<Medicine>>& medicines);

        // Работа с историей заказов (orders.db)
        std::vector<Order> loadOrders();
        void saveOrders(const std::vector<Order>& orders);

        // Бинарное сохранение и загрузка резервной копии каталога (catalog_backup.bin)
        void saveCatalogBinaryBackup(const std::vector<std::shared_ptr<Medicine>>& medicines);
        std::vector<std::shared_ptr<Medicine>> loadCatalogBinaryBackup();

        // Сохранение сводного аналитического отчёта в .txt файл
        void saveTextReport(const std::string& fileName, const std::string& reportContent);

        // Геттеры путей
        inline const std::string& getDataDirectory() const noexcept { return dataDirectory_; }
    };

} // namespace PharmacySystem

#endif // PHARMACY_FILE_MANAGER_H