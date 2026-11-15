#pragma once
#ifndef PHARMACY_USER_H
#define PHARMACY_USER_H

#include <iostream>
#include <string>

namespace PharmacySystem {

    /**
     * @brief Роли пользователей в системе.
     */
    enum class UserRole {
        ADMIN = 1,       // Администратор: полный доступ к каталогу, пользователям, отчётам
        PHARMACIST = 2   // Фармацевт: просмотр, поиск, фильтрация, оформление отпуска/заказа
    };

    /**
     * @brief Класс учётной записи пользователя системы.
     * Хранит логин, соль, хеш пароля и роль. Пароли никогда не хранятся в открытом виде.
     */
    class User {
    private:
        std::string username_;
        std::string passwordHash_;
        std::string salt_;
        UserRole role_;

    public:
        User();
        User(std::string username, std::string passwordHash, std::string salt, UserRole role);

        // Фабричный метод создания нового пользователя с автоматической генерацией соли и хешированием
        static User createWithPlainPassword(const std::string& username,
            const std::string& plainPassword,
            UserRole role);

        // Проверка пароля и смена пароля
        bool verifyPassword(const std::string& plainPassword) const;
        void changePassword(const std::string& newPlainPassword);

        // Геттеры и сеттеры
        inline const std::string& getUsername() const noexcept { return username_; }
        inline const std::string& getPasswordHash() const noexcept { return passwordHash_; }
        inline const std::string& getSalt() const noexcept { return salt_; }
        inline UserRole getRole() const noexcept { return role_; }
        inline void setRole(UserRole newRole) noexcept { role_ = newRole; }

        std::string getRoleDisplayName() const;
        static std::string roleToString(UserRole role);
        static UserRole stringToRole(const std::string& str);

        // Перегрузка оператора сравнения
        bool operator==(const User& other) const;

        // Дружественные операторы потокового ввода/вывода для сериализации в файл
        friend std::ostream& operator<<(std::ostream& os, const User& user);
        friend std::istream& operator>>(std::istream& is, User& user);
    };

} // namespace PharmacySystem

#endif // PHARMACY_USER_H