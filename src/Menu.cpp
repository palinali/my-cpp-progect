#include "Menu.h"
#include "ConsoleUtils.h"
#include "Exceptions.h"
#include "FileManager.h"
#include "MedicineFactory.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

namespace PharmacySystem {

    Menu::Menu() : currentUser_(std::nullopt) {}

    void Menu::initialize() {
        auto& fm = FileManager::getInstance();

        // Загружаем пользователей
        auto loadedUsers = fm.loadUsers();
        for (auto& u : loadedUsers) {
            users_.add(std::move(u));
        }
        seedDefaultUsersIfEmpty();

        // Загружаем каталог лекарств
        auto loadedMeds = fm.loadMedicines();
        for (auto& m : loadedMeds) {
            catalog_.add(std::move(m));
        }
        seedDefaultCatalogIfEmpty();

        // Загружаем историю заказов
        auto loadedOrders = fm.loadOrders();
        for (auto& o : loadedOrders) {
            orders_.add(std::move(o));
        }
    }

    void Menu::seedDefaultUsersIfEmpty() {
        if (users_.empty()) {
            User defaultAdmin = User::createWithPlainPassword("admin", "admin", UserRole::ADMIN);
            User defaultPharm = User::createWithPlainPassword("pharmacist", "1234", UserRole::PHARMACIST);
            users_.add(defaultAdmin);
            users_.add(defaultPharm);

            FileManager::getInstance().saveUsers(users_.getAll());
            FileManager::getInstance().logEvent(LogLevel::INFO,
                "Созданы учётные записи по умолчанию (admin / admin, pharmacist / 1234).");

            std::cout << "\n[СИСТЕМНОЕ УВЕДОМЛЕНИЕ] Файл пользователей не найден или пуст.\n"
                << "Созданы учётные записи по умолчанию:\n"
                << "  * Администратор: логин 'admin', пароль 'admin'\n"
                << "  * Фармацевт:     логин 'pharmacist', пароль '1234'\n\n";
        }
    }

    void Menu::seedDefaultCatalogIfEmpty() {
        if (catalog_.empty()) {
            catalog_.add(MedicineFactory::createMedicine(
                MedicineType::OTC, 0, "Нурофен Экспресс", "Ибупрофен", "Reckitt Benckiser",
                345.50, 42, "2027-08-15", "НПВС / Анальгетики"));

            catalog_.add(MedicineFactory::createMedicine(
                MedicineType::OTC, 0, "Парацетамол 500мг", "Парацетамол", "Фармстандарт",
                85.00, 120, "2028-02-10", "Жаропонижающие"));

            catalog_.add(MedicineFactory::createMedicine(
                MedicineType::PRESCRIPTION, 0, "Амоксиклав 875+125", "Амоксициллин + Клавулановая к-та", "Sandoz",
                520.00, 18, "2027-05-20", "Форма 107-1/у"));

            catalog_.add(MedicineFactory::createMedicine(
                MedicineType::PRESCRIPTION, 0, "Конкор 5мг", "Бисопролол", "Merck",
                410.00, 8, "2027-11-30", "Форма 107-1/у"));

            catalog_.add(MedicineFactory::createMedicine(
                MedicineType::NARCOTIC_PKU, 0, "Трамадол 50мг", "Трамадол", "Органика",
                690.00, 6, "2027-04-01", "ПКУ-148/у-88", 7.5));

            catalog_.add(MedicineFactory::createMedicine(
                MedicineType::OTC, 0, "Супрастин 25мг", "Хлоропирамин", "Egis",
                215.00, 0, "2026-01-10", "Антигистаминные"));

            FileManager::getInstance().saveMedicines(catalog_.getAll());
            FileManager::getInstance().logEvent(LogLevel::INFO,
                "Каталог инициализирован демонстрационными препаратами (6 позиций).");
        }
    }

    void Menu::persistAllData() {
        auto& fm = FileManager::getInstance();
        fm.saveMedicines(catalog_.getAll());
        fm.saveUsers(users_.getAll());
        fm.saveOrders(orders_.getAll());
    }

    void Menu::requireAdminAccess() const {
        if (!currentUser_.has_value() || currentUser_->getRole() != UserRole::ADMIN) {
            throw Exceptions::AuthException("Доступ запрещён! Требуются права Администратора.");
        }
    }

    // ==================== Главный цикл авторизации ====================

    void Menu::run() {
        while (true) {
            Utils::ConsoleUtils::printHeader("АИС АПТЕКИ И АПТЕЧНОГО СКЛАДА — АВТОРИЗАЦИЯ", 90);
            std::cout << "  1. Войти в систему\n"
                << "  0. Выход из программы\n";
            Utils::ConsoleUtils::printSeparator('-', 90);

            int choice = Utils::ConsoleUtils::readNumber<int>("Выберите действие: ", 0, 1);
            if (choice == 0) {
                persistAllData();
                std::cout << "\nВсе данные сохранены. Завершение работы АИС Аптеки.\n";
                break;
            }

            std::string login = Utils::ConsoleUtils::readLine("Логин: ");
            std::string password = Utils::ConsoleUtils::readLine("Пароль: ");

            User* foundUser = users_.findFirst([&login](const User& u) {
                return u.getUsername() == login;
                });

            if (foundUser == nullptr || !foundUser->verifyPassword(password)) {
                FileManager::getInstance().logEvent(LogLevel::WARNING,
                    "Неудачная попытка входа под логином: '" + login + "'");
                std::cout << "\n  [ОШИБКА] Неверный логин или пароль!\n\n";
                continue;
            }

            currentUser_ = *foundUser;
            FileManager::getInstance().logEvent(LogLevel::AUDIT,
                "Вход в систему: " + currentUser_->getUsername() +
                " (" + currentUser_->getRoleDisplayName() + ")");

            std::cout << "\nДобро пожаловать, " << currentUser_->getUsername()
                << " [" << currentUser_->getRoleDisplayName() << "]!\n";

            if (currentUser_->getRole() == UserRole::ADMIN) {
                runAdminMenu();
            }
            else {
                runPharmacistMenu();
            }

            FileManager::getInstance().logEvent(LogLevel::AUDIT,
                "Выход из учётной записи: " + currentUser_->getUsername());
            currentUser_.reset();
        }
    }

    // ==================== Меню Администратора ====================

    void Menu::runAdminMenu() {
        while (currentUser_.has_value()) {
            std::cout << "\n";
            Utils::ConsoleUtils::printHeader("ПАНЕЛЬ АДМИНИСТРАТОРА (" + currentUser_->getUsername() + ")", 90);
            std::cout << "  --- УПРАВЛЕНИЕ КАТАЛОГОМ И СКЛАДОМ ---\n"
                << "  1. Просмотр полного каталога лекарственных средств\n"
                << "  2. Добавить новый препарат в каталог\n"
                << "  3. Редактировать данные препарата\n"
                << "  4. Удалить препарат из каталога\n"
                << "  5. Приход товара на склад (пополнение остатков)\n"
                << "  --- ПОИСК, АНАЛИТИКА И ОТПУСК ---\n"
                << "  6. Поиск препаратов (по названию, МНН, производителю, цене)\n"
                << "  7. Сортировка каталога\n"
                << "  8. Фильтрация (категория, остатки, рецептурность, просрочка)\n"
                << "  9. Оформить отпуск препаратов (создать заказ / чек)\n"
                << " 10. Просмотр истории заказов и повторная печать чека\n"
                << "  --- АДМИНИСТРИРОВАНИЕ И ОТЧЁТНОСТЬ ---\n"
                << " 11. Сформировать сводный отчёт по складу и выручке (экспорт в .txt)\n"
                << " 12. Управление учётными записями пользователей\n"
                << " 13. Резервное копирование (бинарный экспорт / восстановление)\n"
                << "  0. Выйти из учётной записи (смена пользователя)\n";
            Utils::ConsoleUtils::printSeparator('-', 90);

            int choice = Utils::ConsoleUtils::readNumber<int>("Ваш выбор: ", 0, 13);
            try {
                switch (choice) {
                case 1:  viewAllMedicines(); break;
                case 2:  addMedicineInteractive(); break;
                case 3:  editMedicineInteractive(); break;
                case 4:  deleteMedicineInteractive(); break;
                case 5:  restockMedicineInteractive(); break;
                case 6:  handleSearchMenu(); break;
                case 7:  handleSortMenu(); break;
                case 8:  handleFilterMenu(); break;
                case 9:  createOrderInteractive(); break;
                case 10: viewOrderHistory(); break;
                case 11: generateSummaryReportInteractive(); break;
                case 12: manageUsersMenu(); break;
                case 13: manageBackupMenu(); break;
                case 0:
                    persistAllData();
                    return;
                }
            }
            catch (const Exceptions::PharmacyException& ex) {
                std::cout << "\n  [ОШИБКА СИСТЕМЫ] " << ex.what() << "\n";
                FileManager::getInstance().logEvent(LogLevel::ERROR_LEVEL, ex.what());
            }
            catch (const std::exception& ex) {
                std::cout << "\n  [КРИТИЧЕСКАЯ ОШИБКА] " << ex.what() << "\n";
                FileManager::getInstance().logEvent(LogLevel::ERROR_LEVEL, ex.what());
            }
        }
    }

    // ==================== Меню Фармацевта ====================

    void Menu::runPharmacistMenu() {
        while (currentUser_.has_value()) {
            std::cout << "\n";
            Utils::ConsoleUtils::printHeader("РАБОЧЕЕ МЕСТО ФАРМАЦЕВТА (" + currentUser_->getUsername() + ")", 90);
            std::cout << "  1. Просмотр каталога лекарственных средств\n"
                << "  2. Поиск препаратов (по названию, МНН, производителю, цене)\n"
                << "  3. Сортировка каталога\n"
                << "  4. Фильтрация (по категории, наличию, рецепту, сроку годности)\n"
                << "  5. Оформить заказ покупателя / отпуск по рецепту (печать чека)\n"
                << "  6. Просмотр истории оформленных заказов\n"
                << "  7. Сформировать отчёт по остаткам и выручке\n"
                << "  0. Выйти из учётной записи (смена пользователя)\n";
            Utils::ConsoleUtils::printSeparator('-', 90);

            int choice = Utils::ConsoleUtils::readNumber<int>("Ваш выбор: ", 0, 7);
            try {
                switch (choice) {
                case 1: viewAllMedicines(); break;
                case 2: handleSearchMenu(); break;
                case 3: handleSortMenu(); break;
                case 4: handleFilterMenu(); break;
                case 5: createOrderInteractive(); break;
                case 6: viewOrderHistory(); break;
                case 7: generateSummaryReportInteractive(); break;
                case 0:
                    persistAllData();
                    return;
                }
            }
            catch (const Exceptions::PharmacyException& ex) {
                std::cout << "\n  [ОШИБКА СИСТЕМЫ] " << ex.what() << "\n";
                FileManager::getInstance().logEvent(LogLevel::ERROR_LEVEL, ex.what());
            }
            catch (const std::exception& ex) {
                std::cout << "\n  [КРИТИЧЕСКАЯ ОШИБКА] " << ex.what() << "\n";
                FileManager::getInstance().logEvent(LogLevel::ERROR_LEVEL, ex.what());
            }
        }
    }

    // ==================== Отображение таблиц ====================

    void Menu::printMedicinesTable(const std::vector<std::shared_ptr<Medicine>>& list,
        const std::string& tableTitle) const {
        const int tableWidth = 131;
        std::cout << "\n";
        Utils::ConsoleUtils::printHeader(tableTitle + " (Найдено позиций: " + std::to_string(list.size()) + ")", tableWidth);

        if (list.empty()) {
            std::cout << "  Список пуст. Записи, соответствующие критериям, отсутствуют.\n";
            Utils::ConsoleUtils::printSeparator('=', tableWidth);
            return;
        }

        std::cout << "| " << Utils::ConsoleUtils::padRightUTF8("ID", 4)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Название", 20)
            << " | " << Utils::ConsoleUtils::padRightUTF8("МНН / Вещество", 18)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Категория", 16)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Производитель", 15)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Цена(руб)", 9)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Остаток", 10)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Годен до", 11)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Рецепт", 6)
            << " |\n";
        Utils::ConsoleUtils::printSeparator('-', tableWidth);

        for (const auto& med : list) {
            if (med) {
                med->printTableRow(std::cout);
            }
        }

        Utils::ConsoleUtils::printSeparator('=', tableWidth);
        std::cout << "Примечание: (!) — остаток <= 10 уп.; (*) — срок годности истёк.\n";
    }

    void Menu::viewAllMedicines() const {
        printMedicinesTable(catalog_.getAll(), "КАТАЛОГ ЛЕКАРСТВЕННЫХ СРЕДСТВ АПТЕКИ");
    }

    // ==================== CRUD-операции (Администратор) ====================

    void Menu::addMedicineInteractive() {
        requireAdminAccess();
        Utils::ConsoleUtils::printHeader("ДОБАВЛЕНИЕ НОВОГО ЛЕКАРСТВЕННОГО СРЕДСТВА", 80);

        std::cout << "Выберите категорию отпуска препарата:\n"
            << "  1. Безрецептурный препарат (OTC)\n"
            << "  2. Рецептурный препарат (Rx)\n"
            << "  3. Препарат предметно-количественного учёта (ПКУ)\n";
        int typeChoice = Utils::ConsoleUtils::readNumber<int>("Категория (1-3): ", 1, 3);
        MedicineType type = static_cast<MedicineType>(typeChoice);

        std::string name = Utils::ConsoleUtils::readLine("Торговое наименование: ");
        std::string mnn = Utils::ConsoleUtils::readLine("Действующее вещество (МНН): ");
        std::string manufacturer = Utils::ConsoleUtils::readLine("Производитель: ");
        double price = Utils::ConsoleUtils::readNumber<double>("Розничная цена за упаковку (руб.): ", 0.01, 1000000.0);
        int stock = Utils::ConsoleUtils::readNumber<int>("Начальный остаток на складе (уп.): ", 0, 1000000);

        std::string expiry;
        while (true) {
            expiry = Utils::ConsoleUtils::readLine("Срок годности (формат ГГГГ-ММ-ДД): ");
            if (Utils::ConsoleUtils::isValidDate(expiry)) {
                break;
            }
            std::cout << "  [Ошибка] Некорректная дата. Пример правильного ввода: 2027-12-31\n";
        }

        std::string extra1;
        double extra2 = 5.0;

        if (type == MedicineType::OTC) {
            extra1 = Utils::ConsoleUtils::readLine("Фармакотерапевтическая группа (Enter — 'Общая терапия'): ", true);
        }
        else if (type == MedicineType::PRESCRIPTION) {
            extra1 = Utils::ConsoleUtils::readLine("Форма рецептурного бланка (Enter — 'Форма 107-1/у'): ", true);
        }
        else if (type == MedicineType::NARCOTIC_PKU) {
            extra1 = Utils::ConsoleUtils::readLine("Код реестра спецучёта ПКУ (Enter — 'ПКУ-148/у-88'): ", true);
            extra2 = Utils::ConsoleUtils::readNumber<double>("Надбавка за спецучёт и хранение (%): ", 0.0, 100.0);
        }

        auto newMed = MedicineFactory::createMedicine(type, 0, name, mnn, manufacturer, price, stock, expiry, extra1, extra2);
        catalog_.add(newMed);
        FileManager::getInstance().saveMedicines(catalog_.getAll());
        FileManager::getInstance().logEvent(LogLevel::AUDIT,
            "Администратор " + currentUser_->getUsername() +
            " добавил препарат ID=" + std::to_string(newMed->getId()) +
            " ('" + newMed->getName() + "').");

        std::cout << "\n[УСПЕХ] Препарат успешно добавлен в каталог:\n";
        newMed->printDetails(std::cout);
    }

    void Menu::editMedicineInteractive() {
        requireAdminAccess();
        viewAllMedicines();
        if (catalog_.empty()) return;

        int targetId = Utils::ConsoleUtils::readNumber<int>("Введите ID препарата для редактирования (0 — отмена): ", 0);
        if (targetId == 0) return;

        auto* medPtr = catalog_.findFirst([targetId](const std::shared_ptr<Medicine>& m) {
            return m->getId() == targetId;
            });

        if (medPtr == nullptr) {
            throw Exceptions::ValidationException("Редактирование", "Препарат с ID=" + std::to_string(targetId) + " не найден.");
        }

        std::shared_ptr<Medicine> med = *medPtr;
        std::cout << "\nТекущие данные препарата:\n";
        med->printDetails(std::cout);

        std::cout << "\nЧто вы хотите изменить?\n"
            << "  1. Наименование\n"
            << "  2. Действующее вещество (МНН)\n"
            << "  3. Производителя\n"
            << "  4. Розничную цену\n"
            << "  5. Количество на складе\n"
            << "  6. Срок годности\n"
            << "  0. Отмена\n";

        int fieldChoice = Utils::ConsoleUtils::readNumber<int>("Поле для изменения: ", 0, 6);
        if (fieldChoice == 0) return;

        switch (fieldChoice) {
        case 1:
            med->setName(Utils::ConsoleUtils::readLine("Новое наименование: "));
            break;
        case 2:
            med->setActiveIngredient(Utils::ConsoleUtils::readLine("Новое действующее вещество (МНН): "));
            break;
        case 3:
            med->setManufacturer(Utils::ConsoleUtils::readLine("Новый производитель: "));
            break;
        case 4:
            med->setPrice(Utils::ConsoleUtils::readNumber<double>("Новая цена (руб.): ", 0.01, 1000000.0));
            break;
        case 5:
            med->setStockQuantity(Utils::ConsoleUtils::readNumber<int>("Новое количество на складе (уп.): ", 0, 1000000));
            break;
        case 6: {
            std::string newDate = Utils::ConsoleUtils::readLine("Новый срок годности (ГГГГ-ММ-ДД): ");
            med->setExpiryDate(newDate);
            break;
        }
        }

        FileManager::getInstance().saveMedicines(catalog_.getAll());
        FileManager::getInstance().logEvent(LogLevel::AUDIT,
            "Администратор " + currentUser_->getUsername() +
            " отредактировал препарат ID=" + std::to_string(med->getId()));
        std::cout << "\n[УСПЕХ] Данные препарата обновлены!\n";
    }

    void Menu::deleteMedicineInteractive() {
        requireAdminAccess();
        viewAllMedicines();
        if (catalog_.empty()) return;

        int targetId = Utils::ConsoleUtils::readNumber<int>("Введите ID препарата для удаления (0 — отмена): ", 0);
        if (targetId == 0) return;

        auto* medPtr = catalog_.findFirst([targetId](const std::shared_ptr<Medicine>& m) {
            return m->getId() == targetId;
            });

        if (medPtr == nullptr) {
            throw Exceptions::ValidationException("Удаление", "Препарат с ID=" + std::to_string(targetId) + " не найден.");
        }

        std::string medName = (*medPtr)->getName();
        std::cout << "\nВы выбрали для удаления:\n";
        (*medPtr)->printDetails(std::cout);

        if (!Utils::ConsoleUtils::readConfirmation("Вы уверены, что хотите безвозвратно удалить этот препарат?")) {
            std::cout << "Удаление отменено пользователем.\n";
            return;
        }

        catalog_.removeIf([targetId](const std::shared_ptr<Medicine>& m) {
            return m->getId() == targetId;
            });

        FileManager::getInstance().saveMedicines(catalog_.getAll());
        FileManager::getInstance().logEvent(LogLevel::AUDIT,
            "Администратор " + currentUser_->getUsername() +
            " удалил препарат ID=" + std::to_string(targetId) + " ('" + medName + "').");
        std::cout << "\n[УСПЕХ] Препарат '" << medName << "' удалён из каталога.\n";
    }

    void Menu::restockMedicineInteractive() {
        requireAdminAccess();
        viewAllMedicines();
        if (catalog_.empty()) return;

        int targetId = Utils::ConsoleUtils::readNumber<int>("Введите ID препарата для оприходования (0 — отмена): ", 0);
        if (targetId == 0) return;

        auto* medPtr = catalog_.findFirst([targetId](const std::shared_ptr<Medicine>& m) {
            return m->getId() == targetId;
            });

        if (medPtr == nullptr) {
            throw Exceptions::ValidationException("Приход товара", "Препарат с ID=" + std::to_string(targetId) + " не найден.");
        }

        int addQty = Utils::ConsoleUtils::readNumber<int>("Количество поступивших упаковок: ", 1, 100000);
        (*medPtr)->addStock(addQty);

        FileManager::getInstance().saveMedicines(catalog_.getAll());
        FileManager::getInstance().logEvent(LogLevel::AUDIT,
            "Пополнение склада: ID=" + std::to_string(targetId) +
            " (+" + std::to_string(addQty) + " уп.), текущий остаток: " +
            std::to_string((*medPtr)->getStockQuantity()));

        std::cout << "\n[УСПЕХ] Остаток препарата '" << (*medPtr)->getName()
            << "' увеличен. Текущий остаток: " << (*medPtr)->getStockQuantity() << " уп.\n";
    }

    // ==================== Поиск, сортировка, фильтрация ====================

    std::vector<std::shared_ptr<Medicine>> Menu::searchMedicines(const std::string& query) const {
        std::string lowerQuery = Utils::ConsoleUtils::toLowerUTF8(Utils::ConsoleUtils::trim(query));
        return catalog_.findAll([&lowerQuery](const std::shared_ptr<Medicine>& m) {
            std::string nameLow = Utils::ConsoleUtils::toLowerUTF8(m->getName());
            std::string mnnLow = Utils::ConsoleUtils::toLowerUTF8(m->getActiveIngredient());
            std::string manufLow = Utils::ConsoleUtils::toLowerUTF8(m->getManufacturer());
            return nameLow.find(lowerQuery) != std::string::npos ||
                mnnLow.find(lowerQuery) != std::string::npos ||
                manufLow.find(lowerQuery) != std::string::npos;
            });
    }

    std::vector<std::shared_ptr<Medicine>> Menu::searchMedicines(double minPrice, double maxPrice) const {
        return catalog_.findAll([minPrice, maxPrice](const std::shared_ptr<Medicine>& m) {
            return m->getPrice() >= minPrice && m->getPrice() <= maxPrice;
            });
    }

    void Menu::handleSearchMenu() const {
        Utils::ConsoleUtils::printHeader("ПОИСК ЛЕКАРСТВЕННЫХ СРЕДСТВ", 80);
        std::cout << "  1. Поиск по подстроке (Название / МНН / Производитель, без учёта регистра)\n"
            << "  2. Поиск по диапазону розничной цены (от — до)\n"
            << "  3. Поиск по точному ID препарата (с просмотром полной карточки)\n"
            << "  0. Назад\n";

        int choice = Utils::ConsoleUtils::readNumber<int>("Выберите режим поиска: ", 0, 3);
        if (choice == 0) return;

        if (choice == 1) {
            std::string query = Utils::ConsoleUtils::readLine("Введите поисковый запрос: ");
            auto results = searchMedicines(query);
            printMedicinesTable(results, "РЕЗУЛЬТАТЫ ПОИСКА ПО ЗАПРОСУ: \"" + query + "\"");
        }
        else if (choice == 2) {
            double minP = Utils::ConsoleUtils::readNumber<double>("Минимальная цена (руб.): ", 0.0, 1000000.0);
            double maxP = Utils::ConsoleUtils::readNumber<double>("Максимальная цена (руб.): ", minP, 1000000.0);
            auto results = searchMedicines(minP, maxP);
            std::ostringstream title;
            title << "ПРЕПАРАТЫ В ДИАПАЗОНЕ ЦЕН " << std::fixed << std::setprecision(2) << minP << " - " << maxP << " РУБ.";
            printMedicinesTable(results, title.str());
        }
        else if (choice == 3) {
            int id = Utils::ConsoleUtils::readNumber<int>("Введите ID препарата: ", 1);
            const auto* medPtr = catalog_.findFirst([id](const std::shared_ptr<Medicine>& m) {
                return m->getId() == id;
                });
            if (medPtr == nullptr) {
                std::cout << "  Препарат с ID=" << id << " не найден.\n";
            }
            else {
                std::cout << "\n";
                Utils::ConsoleUtils::printSeparator('-', 60);
                (*medPtr)->printDetails(std::cout);
                Utils::ConsoleUtils::printSeparator('-', 60);
            }
        }
    }

    void Menu::handleSortMenu() {
        Utils::ConsoleUtils::printHeader("СОРТИРОВКА КАТАЛОГА", 80);
        std::cout << "  1. По названию (А -> Я)\n"
            << "  2. По цене (по возрастанию)\n"
            << "  3. По цене (по убыванию)\n"
            << "  4. По остатку на складе (по возрастанию — дефицит сверху)\n"
            << "  5. По остатку на складе (по убыванию)\n"
            << "  6. По сроку годности (ближайшие к истечению сверху)\n"
            << "  7. По ID (по возрастанию)\n"
            << "  0. Назад\n";

        int choice = Utils::ConsoleUtils::readNumber<int>("Критерий сортировки: ", 0, 7);
        if (choice == 0) return;

        switch (choice) {
        case 1:
            catalog_.sortBy([](const std::shared_ptr<Medicine>& a, const std::shared_ptr<Medicine>& b) {
                return *a < *b; // Используем перегруженный operator<
                });
            break;
        case 2:
            catalog_.sortBy([](const std::shared_ptr<Medicine>& a, const std::shared_ptr<Medicine>& b) {
                return a->getPrice() < b->getPrice();
                });
            break;
        case 3:
            catalog_.sortBy([](const std::shared_ptr<Medicine>& a, const std::shared_ptr<Medicine>& b) {
                return a->getPrice() > b->getPrice();
                });
            break;
        case 4:
            catalog_.sortBy([](const std::shared_ptr<Medicine>& a, const std::shared_ptr<Medicine>& b) {
                return a->getStockQuantity() < b->getStockQuantity();
                });
            break;
        case 5:
            catalog_.sortBy([](const std::shared_ptr<Medicine>& a, const std::shared_ptr<Medicine>& b) {
                return a->getStockQuantity() > b->getStockQuantity();
                });
            break;
        case 6:
            catalog_.sortBy([](const std::shared_ptr<Medicine>& a, const std::shared_ptr<Medicine>& b) {
                return a->getExpiryDate() < b->getExpiryDate();
                });
            break;
        case 7:
            catalog_.sortBy([](const std::shared_ptr<Medicine>& a, const std::shared_ptr<Medicine>& b) {
                return a->getId() < b->getId();
                });
            break;
        }

        viewAllMedicines();
    }

    void Menu::handleFilterMenu() const {
        Utils::ConsoleUtils::printHeader("ФИЛЬТРАЦИЯ КАТАЛОГА", 80);
        std::cout << "  1. Только в наличии (остаток > 0 и не просрочены)\n"
            << "  2. Заканчивающиеся препараты (остаток от 1 до 10 уп.)\n"
            << "  3. Отсутствующие на складе (остаток == 0)\n"
            << "  4. Просроченные препараты (требуют списания)\n"
            << "  5. Только безрецептурные (OTC)\n"
            << "  6. Только рецептурные (Rx и ПКУ)\n"
            << "  7. Только препараты предметно-количественного учёта (ПКУ)\n"
            << "  0. Назад\n";

        int choice = Utils::ConsoleUtils::readNumber<int>("Выберите фильтр: ", 0, 7);
        if (choice == 0) return;

        std::vector<std::shared_ptr<Medicine>> filtered;
        std::string title;

        switch (choice) {
        case 1:
            title = "ПРЕПАРАТЫ В НАЛИЧИИ (ГОДНЫЕ К ОТПУСКУ)";
            filtered = catalog_.findAll([](const std::shared_ptr<Medicine>& m) {
                return m->getStockQuantity() > 0 && !m->isExpired();
                });
            break;
        case 2:
            title = "ЗАКАНЧИВАЮЩИЕСЯ ПОЗИЦИИ (ОСТАТОК <= 10 УП.)";
            filtered = catalog_.findAll([](const std::shared_ptr<Medicine>& m) {
                return m->isLowStock(10);
                });
            break;
        case 3:
            title = "ОТСУТСТВУЮЩИЕ НА СКЛАДЕ (ОСТАТОК = 0)";
            filtered = catalog_.findAll([](const std::shared_ptr<Medicine>& m) {
                return m->getStockQuantity() == 0;
                });
            break;
        case 4:
            title = "ПРОСРОЧЕННЫЕ ПРЕПАРАТЫ (КАРАНТИН / СПИСАНИЕ)";
            filtered = catalog_.findAll([](const std::shared_ptr<Medicine>& m) {
                return m->isExpired();
                });
            break;
        case 5:
            title = "БЕЗРЕЦЕПТУРНЫЕ ПРЕПАРАТЫ (OTC)";
            filtered = catalog_.findAll([](const std::shared_ptr<Medicine>& m) {
                return !m->requiresPrescription();
                });
            break;
        case 6:
            title = "РЕЦЕПТУРНЫЕ ПРЕПАРАТЫ (Rx И ПКУ)";
            filtered = catalog_.findAll([](const std::shared_ptr<Medicine>& m) {
                return m->requiresPrescription();
                });
            break;
        case 7:
            title = "ПРЕПАРАТЫ ПРЕДМЕТНО-КОЛИЧЕСТВЕННОГО УЧЁТА (ПКУ)";
            filtered = catalog_.findAll([](const std::shared_ptr<Medicine>& m) {
                return m->getType() == MedicineType::NARCOTIC_PKU;
                });
            break;
        }

        printMedicinesTable(filtered, title);
    }

    // ==================== Оформление заказов и отчётов ====================

    void Menu::createOrderInteractive() {
        Utils::ConsoleUtils::printHeader("ОФОРМЛЕНИЕ ЗАКАЗА ПОКУПАТЕЛЯ / ОТПУСК ПРЕПАРАТОВ", 90);
        Order currentOrder(currentUser_->getUsername());

        while (true) {
            std::cout << "\nТекущая корзина: позиций = " << currentOrder.getItems().size()
                << " | Предварительная сумма = " << std::fixed << std::setprecision(2)
                << currentOrder.getTotalAmount() << " руб.\n"
                << "  1. Добавить препарат в заказ по ID\n"
                << "  2. Показать каталог доступных препаратов\n"
                << "  3. Посмотреть текущий состав чека\n"
                << "  4. Провести заказ и сформировать чек\n"
                << "  0. Отменить заказ и очистить корзину\n";

            int action = Utils::ConsoleUtils::readNumber<int>("Действие: ", 0, 4);

            if (action == 0) {
                std::cout << "Оформление заказа отменено.\n";
                return;
            }
            else if (action == 2) {
                viewAllMedicines();
            }
            else if (action == 3) {
                std::cout << "\n" << currentOrder;
            }
            else if (action == 1) {
                int medId = Utils::ConsoleUtils::readNumber<int>("Введите ID препарата: ", 1);
                auto* medPtr = catalog_.findFirst([medId](const std::shared_ptr<Medicine>& m) {
                    return m->getId() == medId;
                    });

                if (medPtr == nullptr) {
                    std::cout << "  [Ошибка] Препарат с ID=" << medId << " не найден в каталоге.\n";
                    continue;
                }

                std::shared_ptr<Medicine> med = *medPtr;
                std::cout << "  Выбран: " << med->getName()
                    << " | Категория: " << med->getCategoryName()
                    << " | Цена: " << std::fixed << std::setprecision(2) << med->getPrice() << " руб."
                    << " | Доступно: " << med->getStockQuantity() << " уп.\n"
                    << "  Правило отпуска: " << med->getDispenseRule() << "\n";

                int qty = Utils::ConsoleUtils::readNumber<int>("Введите количество упаковок: ", 1, 10000);
                try {
                    currentOrder.addMedicineItem(med, qty);
                    std::cout << "  [ОК] Позиция добавлена в корзину.\n";
                }
                catch (const Exceptions::PharmacyException& ex) {
                    std::cout << "  [ОШИБКА ДОБАВЛЕНИЯ] " << ex.what() << "\n";
                }
            }
            else if (action == 4) {
                if (currentOrder.isEmpty()) {
                    std::cout << "  [Ошибка] Корзина пуста. Добавьте хотя бы один препарат.\n";
                    continue;
                }

                if (currentOrder.hasPrescriptionItems()) {
                    std::cout << "\n[ВНИМАНИЕ] В заказе присутствуют рецептурные препараты или препараты ПКУ!\n";
                    std::string rxInfo = Utils::ConsoleUtils::readLine("Введите серию, номер рецепта и ФИО врача: ");
                    currentOrder.setPrescriptionDetails(rxInfo);
                }

                // Проводим заказ (списываем остатки со склада)
                currentOrder.finalizeOrder(catalog_.getAll());
                orders_.add(currentOrder);

                // Сохраняем обновлённый каталог и историю заказов
                FileManager::getInstance().saveMedicines(catalog_.getAll());
                FileManager::getInstance().saveOrders(orders_.getAll());
                FileManager::getInstance().logEvent(LogLevel::AUDIT,
                    "Проведён заказ №" + std::to_string(currentOrder.getOrderId()) +
                    " на сумму " + std::to_string(currentOrder.getTotalAmount()) +
                    " руб. (фармацевт: " + currentUser_->getUsername() + ")");

                std::cout << "\n[УСПЕХ] Заказ успешно проведён! Складские остатки обновлены.\n\n";
                std::cout << currentOrder; // Вывод чека через перегруженный operator<<

                if (Utils::ConsoleUtils::readConfirmation("Сохранить кассовый чек в отдельный текстовый файл (.txt)?")) {
                    std::string fileName = FileManager::getInstance().getDataDirectory() +
                        "/receipt_" + std::to_string(currentOrder.getOrderId()) + ".txt";
                    if (currentOrder.exportReceiptToTxt(fileName)) {
                        std::cout << "  [ОК] Чек сохранён в файл: " << fileName << "\n";
                    }
                }
                return;
            }
        }
    }

    void Menu::viewOrderHistory() const {
        Utils::ConsoleUtils::printHeader("ИСТОРИЯ ПРОВЕДЁННЫХ ЗАКАЗОВ", 90);
        if (orders_.empty()) {
            std::cout << "  История заказов пока пуста.\n";
            return;
        }

        std::cout << "| " << Utils::ConsoleUtils::padRightUTF8("№ Чека", 8)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Дата и время", 20)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Сотрудник", 15)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Позиций", 8)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Итого (руб.)", 14)
            << " |\n";
        Utils::ConsoleUtils::printSeparator('-', 81);

        for (const auto& ord : orders_.getAll()) {
            std::ostringstream sumSs;
            sumSs << std::fixed << std::setprecision(2) << ord.getTotalAmount();
            std::cout << "| " << Utils::ConsoleUtils::padRightUTF8(std::to_string(ord.getOrderId()), 8)
                << " | " << Utils::ConsoleUtils::padRightUTF8(ord.getTimestamp(), 20)
                << " | " << Utils::ConsoleUtils::padRightUTF8(ord.getPharmacistUsername(), 15)
                << " | " << Utils::ConsoleUtils::padRightUTF8(std::to_string(ord.getItems().size()), 8)
                << " | " << Utils::ConsoleUtils::padRightUTF8(sumSs.str(), 14)
                << " |\n";
        }
        Utils::ConsoleUtils::printSeparator('=', 81);

        int inspectId = Utils::ConsoleUtils::readNumber<int>(
            "Введите № чека для детального просмотра и экспорта (0 — выход): ", 0);
        if (inspectId == 0) return;

        const Order* foundOrder = orders_.findFirst([inspectId](const Order& o) {
            return o.getOrderId() == inspectId;
            });

        if (foundOrder == nullptr) {
            std::cout << "  Заказ №" << inspectId << " не найден.\n";
            return;
        }

        std::cout << "\n" << *foundOrder;
        if (Utils::ConsoleUtils::readConfirmation("Экспортировать этот чек в файл .txt?")) {
            std::string fileName = FileManager::getInstance().getDataDirectory() +
                "/receipt_" + std::to_string(foundOrder->getOrderId()) + ".txt";
            if (foundOrder->exportReceiptToTxt(fileName)) {
                std::cout << "  [ОК] Чек сохранён: " << fileName << "\n";
            }
        }
    }

    // ==================== Управление пользователями (Администратор) ====================

    void Menu::manageUsersMenu() {
        requireAdminAccess();
        while (true) {
            std::cout << "\n";
            Utils::ConsoleUtils::printHeader("УПРАВЛЕНИЕ УЧЁТНЫМИ ЗАПИСЯМИ ПОЛЬЗОВАТЕЛЕЙ", 80);
            std::cout << "  1. Список зарегистрированных сотрудников\n"
                << "  2. Зарегистрировать нового пользователя (Администратор / Фармацевт)\n"
                << "  3. Сменить пароль пользователя\n"
                << "  4. Удалить учётную запись пользователя\n"
                << "  0. Назад в меню Администратора\n";

            int choice = Utils::ConsoleUtils::readNumber<int>("Ваш выбор: ", 0, 4);
            switch (choice) {
            case 1: viewUsersList(); break;
            case 2: registerNewUserInteractive(); break;
            case 3: changeUserPasswordInteractive(); break;
            case 4: deleteUserInteractive(); break;
            case 0: return;
            }
        }
    }

    void Menu::viewUsersList() const {
        Utils::ConsoleUtils::printHeader("СПИСОК ПОЛЬЗОВАТЕЛЕЙ СИСТЕМЫ", 75);
        std::cout << "| " << Utils::ConsoleUtils::padRightUTF8("Логин", 20)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Роль в системе", 20)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Хеш пароля (превью)", 24)
            << " |\n";
        Utils::ConsoleUtils::printSeparator('-', 74);

        for (const auto& u : users_.getAll()) {
            std::string hashPreview = u.getPasswordHash().substr(0, 16) + "...";
            std::cout << "| " << Utils::ConsoleUtils::padRightUTF8(u.getUsername(), 20)
                << " | " << Utils::ConsoleUtils::padRightUTF8(u.getRoleDisplayName(), 20)
                << " | " << Utils::ConsoleUtils::padRightUTF8(hashPreview, 24)
                << " |\n";
        }
        Utils::ConsoleUtils::printSeparator('=', 74);
    }

    void Menu::registerNewUserInteractive() {
        std::string newLogin = Utils::ConsoleUtils::readLine("Введите логин нового сотрудника: ");
        if (users_.findFirst([&newLogin](const User& u) { return u.getUsername() == newLogin; }) != nullptr) {
            throw Exceptions::ValidationException("Регистрация", "Пользователь с логином '" + newLogin + "' уже существует.");
        }

        std::string newPass = Utils::ConsoleUtils::readLine("Введите пароль (минимум 4 символа): ");
        std::cout << "Выберите роль:\n  1. Администратор\n  2. Фармацевт\n";
        int roleChoice = Utils::ConsoleUtils::readNumber<int>("Роль (1-2): ", 1, 2);
        UserRole role = (roleChoice == 1) ? UserRole::ADMIN : UserRole::PHARMACIST;

        User created = User::createWithPlainPassword(newLogin, newPass, role);
        users_.add(created);
        FileManager::getInstance().saveUsers(users_.getAll());
        FileManager::getInstance().logEvent(LogLevel::AUDIT,
            "Создан пользователь '" + newLogin + "' с ролью " + created.getRoleDisplayName());
        std::cout << "\n[УСПЕХ] Пользователь '" << newLogin << "' зарегистрирован.\n";
    }

    void Menu::changeUserPasswordInteractive() {
        viewUsersList();
        std::string targetLogin = Utils::ConsoleUtils::readLine("Введите логин пользователя для смены пароля: ");
        User* targetUser = users_.findFirst([&targetLogin](const User& u) { return u.getUsername() == targetLogin; });
        if (targetUser == nullptr) {
            throw Exceptions::ValidationException("Смена пароля", "Пользователь '" + targetLogin + "' не найден.");
        }

        std::string newPass = Utils::ConsoleUtils::readLine("Введите новый пароль (минимум 4 символа): ");
        targetUser->changePassword(newPass);

        if (currentUser_.has_value() && currentUser_->getUsername() == targetLogin) {
            currentUser_ = *targetUser;
        }

        FileManager::getInstance().saveUsers(users_.getAll());
        FileManager::getInstance().logEvent(LogLevel::AUDIT, "Изменён пароль для пользователя '" + targetLogin + "'.");
        std::cout << "\n[УСПЕХ] Пароль пользователя '" << targetLogin << "' успешно обновлён.\n";
    }

    void Menu::deleteUserInteractive() {
        viewUsersList();
        std::string targetLogin = Utils::ConsoleUtils::readLine("Введите логин пользователя для удаления: ");

        if (currentUser_.has_value() && currentUser_->getUsername() == targetLogin) {
            throw Exceptions::AuthException("Нельзя удалить собственную активную учётную запись.");
        }

        User* targetUser = users_.findFirst([&targetLogin](const User& u) { return u.getUsername() == targetLogin; });
        if (targetUser == nullptr) {
            throw Exceptions::ValidationException("Удаление пользователя", "Пользователь '" + targetLogin + "' не найден.");
        }

        if (!Utils::ConsoleUtils::readConfirmation("Удалить учётную запись '" + targetLogin + "'?")) {
            return;
        }

        users_.removeIf([&targetLogin](const User& u) { return u.getUsername() == targetLogin; });
        FileManager::getInstance().saveUsers(users_.getAll());
        FileManager::getInstance().logEvent(LogLevel::AUDIT, "Удалена учётная запись пользователя '" + targetLogin + "'.");
        std::cout << "\n[УСПЕХ] Пользователь '" << targetLogin << "' удалён.\n";
    }

    // ==================== Сводная аналитика и резервное копирование ====================

    void Menu::generateSummaryReportInteractive() const {
        int totalPositions = static_cast<int>(catalog_.size());
        int totalPackages = 0;
        double totalStockValuation = 0.0;
        int expiredCount = 0;
        int lowStockCount = 0;
        int outOfStockCount = 0;

        // Используем std::map для группировки статистики по категориям препаратов
        std::map<std::string, int> categoryPositionsCount;
        std::map<std::string, double> categoryStockValue;

        for (const auto& med : catalog_.getAll()) {
            int qty = med->getStockQuantity();
            double val = med->getPrice() * static_cast<double>(qty);

            totalPackages += qty;
            totalStockValuation += val;

            if (med->isExpired()) ++expiredCount;
            if (qty == 0) ++outOfStockCount;
            else if (med->isLowStock(10)) ++lowStockCount;

            std::string cat = med->getCategoryName();
            categoryPositionsCount[cat] += 1;
            categoryStockValue[cat] += val;
        }

        std::ostringstream report;
        std::string eq(82, '=');
        std::string dash(82, '-');

        report << eq << "\n"
            << "          СВОДНЫЙ АНАЛИТИЧЕСКИЙ ОТЧЁТ ПО СКЛАДУ И ВЫРУЧКЕ АИС АПТЕКИ\n"
            << eq << "\n"
            << "Дата и время формирования:   " << Utils::ConsoleUtils::getCurrentDateTime() << "\n"
            << "Сформировал сотрудник:       " << currentUser_->getUsername()
            << " (" << currentUser_->getRoleDisplayName() << ")\n"
            << "Активных объектов в памяти:  " << MedicalEntity::getActiveObjectCount() << " экз.\n"
            << dash << "\n"
            << "1. СОСТОЯНИЕ ТОВАРНЫХ ЗАПАСОВ:\n"
            << "   * Наименований в каталоге:        " << totalPositions << " поз.\n"
            << "   * Общий объём товарных остатков:  " << totalPackages << " уп.\n"
            << "   * Оценочная стоимость склада:     " << std::fixed << std::setprecision(2)
            << totalStockValuation << " руб.\n"
            << "   * Заканчивающиеся позиции (<=10): " << lowStockCount << " поз.\n"
            << "   * Отсутствующие на складе (0):    " << outOfStockCount << " поз.\n"
            << "   * Просроченные препараты:         " << expiredCount << " поз.\n"
            << dash << "\n"
            << "2. РАЗБИВКА ПО КАТЕГОРИЯМ ПРЕПАРАТОВ:\n";

        for (const auto& [category, count] : categoryPositionsCount) {
            report << "   * " << Utils::ConsoleUtils::padRightUTF8(category, 22)
                << ": " << count << " поз. | Стоимость запаса: "
                << std::fixed << std::setprecision(2) << categoryStockValue[category] << " руб.\n";
        }

        report << dash << "\n"
            << "3. ФИНАНСОВЫЕ ПОКАЗАТЕЛИ РЕАЛИЗАЦИИ:\n"
            << "   * Всего оформлено заказов/чеков:  " << orders_.size() << " шт.\n"
            << "   * Совокупная выручка по чекам:    " << std::fixed << std::setprecision(2)
            << Order::getTotalAccumulatedRevenue() << " руб.\n"
            << eq << "\n";

        std::cout << "\n" << report.str();

        if (Utils::ConsoleUtils::readConfirmation("Сохранить сводный отчёт в текстовый файл (.txt)?")) {
            std::string fileName = "summary_report_" + Utils::ConsoleUtils::getCurrentDate() + ".txt";
            FileManager::getInstance().saveTextReport(fileName, report.str());
            std::cout << "  [ОК] Отчёт сохранён в файл: "
                << FileManager::getInstance().getDataDirectory() << "/" << fileName << "\n";
        }
    }

    void Menu::manageBackupMenu() {
        requireAdminAccess();
        Utils::ConsoleUtils::printHeader("РЕЗЕРВНОЕ КОПИРОВАНИЕ (БИНАРНЫЙ ФОРМАТ)", 80);
        std::cout << "  1. Создать бинарную резервную копию каталога (catalog_backup.bin)\n"
            << "  2. Восстановить каталог из бинарной резервной копии\n"
            << "  0. Назад\n";

        int choice = Utils::ConsoleUtils::readNumber<int>("Ваш выбор: ", 0, 2);
        if (choice == 0) return;

        if (choice == 1) {
            FileManager::getInstance().saveCatalogBinaryBackup(catalog_.getAll());
            std::cout << "\n[УСПЕХ] Бинарная резервная копия каталога успешно создана.\n";
        }
        else if (choice == 2) {
            if (!Utils::ConsoleUtils::readConfirmation("Текущий каталог в памяти будет заменён данными из бэкапа. Продолжить?")) {
                return;
            }
            auto restored = FileManager::getInstance().loadCatalogBinaryBackup();
            catalog_.clear();
            for (auto& item : restored) {
                catalog_.add(std::move(item));
            }
            FileManager::getInstance().saveMedicines(catalog_.getAll());
            std::cout << "\n[УСПЕХ] Каталог восстановлен из бинарного файла (" << catalog_.size() << " поз.).\n";
        }
    }

} // namespace PharmacySystem