#include "FileManager.h"
#include "ConsoleUtils.h"
#include "Exceptions.h"
#include "MedicineFactory.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace PharmacySystem {

    FileManager::FileManager()
        : dataDirectory_("data"),
        medicinesFilePath_("data/medicines.db"),
        usersFilePath_("data/users.db"),
        ordersFilePath_("data/orders.db"),
        logFilePath_("data/audit.log"),
        binaryBackupPath_("data/catalog_backup.bin") {
        ensureDirectoryExists();
    }

    FileManager& FileManager::getInstance() {
        static FileManager instance;
        return instance;
    }

    void FileManager::ensureDirectoryExists() const {
        std::error_code ec;
        if (!fs::exists(dataDirectory_, ec)) {
            fs::create_directories(dataDirectory_, ec);
        }
    }

    void FileManager::logEvent(LogLevel level, const std::string& message) {
        ensureDirectoryExists();
        std::ofstream logStream(logFilePath_, std::ios::out | std::ios::app);
        if (!logStream.is_open()) {
            return; // Не прерываем работу программы при недоступности лога
        }

        std::string levelStr;
        switch (level) {
        case LogLevel::INFO:        levelStr = "INFO "; break;
        case LogLevel::WARNING:     levelStr = "WARN "; break;
        case LogLevel::ERROR_LEVEL: levelStr = "ERROR"; break;
        case LogLevel::AUDIT:       levelStr = "AUDIT"; break;
        }

        logStream << "[" << Utils::ConsoleUtils::getCurrentDateTime() << "] "
            << "[" << levelStr << "] "
            << message << "\n";
    }

    // ==================== Пользователи ====================

    std::vector<User> FileManager::loadUsers() {
        ensureDirectoryExists();
        std::vector<User> users;

        std::ifstream inFile(usersFilePath_);
        if (!inFile.is_open()) {
            // Если файл ещё не создан — возвращаем пустой вектор (будет создан admin по умолчанию)
            return users;
        }

        while (inFile.peek() != std::ifstream::traits_type::eof()) {
            User u;
            if (inFile >> u) {
                users.push_back(std::move(u));
            }
            else {
                if (inFile.eof()) break;
                inFile.clear();
            }
        }

        logEvent(LogLevel::INFO, "Загружено учётных записей пользователей: " + std::to_string(users.size()));
        return users;
    }

    void FileManager::saveUsers(const std::vector<User>& users) {
        ensureDirectoryExists();
        std::ofstream outFile(usersFilePath_, std::ios::out | std::ios::trunc);
        if (!outFile.is_open() || !outFile.good()) {
            throw Exceptions::FileAccessException(usersFilePath_, "Невозможно открыть файл для записи пользователей.");
        }

        for (const auto& user : users) {
            outFile << user << "\n";
        }

        if (!outFile.good()) {
            throw Exceptions::FileAccessException(usersFilePath_, "Ошибка при записи данных пользователей на диск.");
        }
        logEvent(LogLevel::AUDIT, "Сохранена база пользователей (" + std::to_string(users.size()) + " записей).");
    }

    // ==================== Лекарственные средства ====================

    std::vector<std::shared_ptr<Medicine>> FileManager::loadMedicines() {
        ensureDirectoryExists();
        std::vector<std::shared_ptr<Medicine>> medicines;

        std::ifstream inFile(medicinesFilePath_);
        if (!inFile.is_open()) {
            return medicines;
        }

        std::string line;
        int lineNumber = 0;
        while (std::getline(inFile, line)) {
            ++lineNumber;
            line = Utils::ConsoleUtils::trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }

            try {
                auto med = MedicineFactory::createFromSerializedString(line);
                medicines.push_back(std::move(med));
            }
            catch (const std::exception& ex) {
                logEvent(LogLevel::WARNING,
                    "Пропущена некорректная запись в " + medicinesFilePath_ +
                    " (строка " + std::to_string(lineNumber) + "): " + ex.what());
            }
        }

        logEvent(LogLevel::INFO, "Загружено препаратов из каталога: " + std::to_string(medicines.size()));
        return medicines;
    }

    void FileManager::saveMedicines(const std::vector<std::shared_ptr<Medicine>>& medicines) {
        ensureDirectoryExists();
        std::ofstream outFile(medicinesFilePath_, std::ios::out | std::ios::trunc);
        if (!outFile.is_open() || !outFile.good()) {
            throw Exceptions::FileAccessException(medicinesFilePath_, "Невозможно открыть файл каталога для записи.");
        }

        outFile << "# Формат: TYPE|ID|NAME|MNN|MANUFACTURER|PRICE|STOCK|EXPIRY|EXTRA1|EXTRA2\n";
        for (const auto& med : medicines) {
            if (med) {
                outFile << *med << "\n";
            }
        }

        if (!outFile.good()) {
            throw Exceptions::FileAccessException(medicinesFilePath_, "Ошибка записи каталога препаратов.");
        }
        logEvent(LogLevel::AUDIT, "Каталог лекарственных средств сохранён (" + std::to_string(medicines.size()) + " поз.).");
    }

    // ==================== История заказов ====================

    std::vector<Order> FileManager::loadOrders() {
        ensureDirectoryExists();
        std::vector<Order> orders;
        Order::resetTotalRevenue();

        std::ifstream inFile(ordersFilePath_);
        if (!inFile.is_open()) {
            return orders;
        }

        std::string line;
        int lineNumber = 0;
        while (std::getline(inFile, line)) {
            ++lineNumber;
            line = Utils::ConsoleUtils::trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }

            try {
                Order ord = Order::deserialize(line);
                Order::addToTotalRevenue(ord.getTotalAmount());
                orders.push_back(std::move(ord));
            }
            catch (const std::exception& ex) {
                logEvent(LogLevel::WARNING,
                    "Ошибка чтения заказа в " + ordersFilePath_ +
                    " (строка " + std::to_string(lineNumber) + "): " + ex.what());
            }
        }

        logEvent(LogLevel::INFO, "Загружено заказов из истории: " + std::to_string(orders.size()));
        return orders;
    }

    void FileManager::saveOrders(const std::vector<Order>& orders) {
        ensureDirectoryExists();
        std::ofstream outFile(ordersFilePath_, std::ios::out | std::ios::trunc);
        if (!outFile.is_open() || !outFile.good()) {
            throw Exceptions::FileAccessException(ordersFilePath_, "Невозможно открыть файл истории заказов для записи.");
        }

        for (const auto& order : orders) {
            outFile << order.serialize() << "\n";
        }

        if (!outFile.good()) {
            throw Exceptions::FileAccessException(ordersFilePath_, "Ошибка записи истории заказов.");
        }
        logEvent(LogLevel::AUDIT, "История заказов сохранена (" + std::to_string(orders.size()) + " чеков).");
    }

    // ==================== Бинарный бэкап каталога ====================

    void FileManager::saveCatalogBinaryBackup(const std::vector<std::shared_ptr<Medicine>>& medicines) {
        ensureDirectoryExists();
        std::ofstream binOut(binaryBackupPath_, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!binOut.is_open() || !binOut.good()) {
            throw Exceptions::FileAccessException(binaryBackupPath_, "Не удалось открыть бинарный файл резервной копии.");
        }

        // Записываем сигнатуру формата ("PHRM") и количество записей
        const char magic[4] = { 'P', 'H', 'R', 'M' };
        binOut.write(magic, 4);

        uint32_t count = static_cast<uint32_t>(medicines.size());
        binOut.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& med : medicines) {
            std::string serialized = med->serialize();
            uint32_t len = static_cast<uint32_t>(serialized.size());
            binOut.write(reinterpret_cast<const char*>(&len), sizeof(len));
            binOut.write(serialized.data(), static_cast<std::streamsize>(len));
        }

        if (!binOut.good()) {
            throw Exceptions::FileAccessException(binaryBackupPath_, "Сбой при записи бинарного потока.");
        }
        logEvent(LogLevel::AUDIT, "Создана бинарная резервная копия каталога: " + binaryBackupPath_);
    }

    std::vector<std::shared_ptr<Medicine>> FileManager::loadCatalogBinaryBackup() {
        std::ifstream binIn(binaryBackupPath_, std::ios::in | std::ios::binary);
        if (!binIn.is_open()) {
            throw Exceptions::FileAccessException(binaryBackupPath_, "Файл бинарной резервной копии не найден.");
        }

        char magic[4]{};
        binIn.read(magic, 4);
        if (!binIn.good() || magic[0] != 'P' || magic[1] != 'H' || magic[2] != 'R' || magic[3] != 'M') {
            throw Exceptions::FileAccessException(binaryBackupPath_, "Неверная сигнатура бинарного файла.");
        }

        uint32_t count = 0;
        binIn.read(reinterpret_cast<char*>(&count), sizeof(count));

        std::vector<std::shared_ptr<Medicine>> restored;
        restored.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t len = 0;
            binIn.read(reinterpret_cast<char*>(&len), sizeof(len));
            if (!binIn.good() || len > 10000) {
                throw Exceptions::FileAccessException(binaryBackupPath_, "Повреждена запись в бинарном файле.");
            }

            std::string record(len, '\0');
            binIn.read(&record[0], static_cast<std::streamsize>(len));
            if (!binIn.good()) {
                throw Exceptions::FileAccessException(binaryBackupPath_, "Неожиданный конец бинарного файла.");
            }

            restored.push_back(MedicineFactory::createFromSerializedString(record));
        }

        logEvent(LogLevel::AUDIT, "Каталог восстановлен из бинарного бэкапа (" + std::to_string(restored.size()) + " поз.).");
        return restored;
    }

    void FileManager::saveTextReport(const std::string& fileName, const std::string& reportContent) {
        ensureDirectoryExists();
        std::string fullPath = dataDirectory_ + "/" + fileName;
        std::ofstream out(fullPath, std::ios::out | std::ios::trunc);
        if (!out.is_open() || !out.good()) {
            throw Exceptions::FileAccessException(fullPath, "Не удалось сохранить текстовый отчёт.");
        }
        out << reportContent;
        if (!out.good()) {
            throw Exceptions::FileAccessException(fullPath, "Ошибка при записи текстового отчёта.");
        }
        logEvent(LogLevel::INFO, "Экспортирован текстовый отчёт: " + fullPath);
    }

} // namespace PharmacySystem