#include "ConsoleUtils.h"
#include "Exceptions.h"
#include "FileManager.h"
#include "Menu.h"
#include <iostream>

int main() {
    // 1. Настраиваем кодировку UTF-8 и локаль для корректного отображения кириллицы (Windows 10+ / macOS / Linux)
    Utils::ConsoleUtils::initConsoleUTF8();

    try {
        PharmacySystem::FileManager::getInstance().logEvent(
            PharmacySystem::LogLevel::INFO, "=== Запуск приложения АИС Аптеки ===");

        // 2. Инициализируем контроллер приложения и загружаем базы данных
        PharmacySystem::Menu appMenu;
        appMenu.initialize();

        // 3. Запускаем главный интерактивный цикл
        appMenu.run();

        PharmacySystem::FileManager::getInstance().logEvent(
            PharmacySystem::LogLevel::INFO, "=== Штатное завершение приложения АИС Аптеки ===");
        return 0;

    }
    catch (const Exceptions::PharmacyException& ex) {
        std::cerr << "\n[КРИТИЧЕСКАЯ ОШИБКА ПРИКЛАДНОГО УРОВНЯ]\n"
            << ex.what() << "\n";
        PharmacySystem::FileManager::getInstance().logEvent(
            PharmacySystem::LogLevel::ERROR_LEVEL, std::string("Критическое исключение: ") + ex.what());
        return 1;

    }
    catch (const std::exception& ex) {
        std::cerr << "\n[НЕПРЕДВИДЕННАЯ СИСТЕМНАЯ ОШИБКА]\n"
            << ex.what() << "\n";
        PharmacySystem::FileManager::getInstance().logEvent(
            PharmacySystem::LogLevel::ERROR_LEVEL, std::string("Системное исключение std::exception: ") + ex.what());
        return 2;

    }
    catch (...) {
        std::cerr << "\n[ФАТАЛЬНАЯ ОШИБКА] Произошло неизвестное исключение.\n";
        PharmacySystem::FileManager::getInstance().logEvent(
            PharmacySystem::LogLevel::ERROR_LEVEL, "Неизвестное исключение (...) в main().");
        return 3;
    }
}