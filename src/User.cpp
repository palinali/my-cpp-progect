#include "User.h"
#include "ConsoleUtils.h"
#include "Exceptions.h"
#include "Security.h"
#include <utility>
#include <vector>

namespace PharmacySystem {

    User::User()
        : username_(""), passwordHash_(""), salt_(""), role_(UserRole::PHARMACIST) {
    }

    User::User(std::string username, std::string passwordHash, std::string salt, UserRole role)
        : username_(std::move(username)),
        passwordHash_(std::move(passwordHash)),
        salt_(std::move(salt)),
        role_(role) {
    }

    User User::createWithPlainPassword(const std::string& username,
        const std::string& plainPassword,
        UserRole role) {
        if (username.empty() || username.find('|') != std::string::npos) {
            throw Exceptions::ValidationException("Логин", "Логин не может быть пустым или содержать '|'.");
        }
        if (plainPassword.size() < 4) {
            throw Exceptions::ValidationException("Пароль", "Пароль должен содержать минимум 4 символа.");
        }

        std::string salt = Security::PasswordHasher::generateSalt(16);
        std::string hash = Security::PasswordHasher::hashPassword(plainPassword, salt);
        return User(username, hash, salt, role);
    }

    bool User::verifyPassword(const std::string& plainPassword) const {
        return Security::PasswordHasher::verifyPassword(plainPassword, salt_, passwordHash_);
    }

    void User::changePassword(const std::string& newPlainPassword) {
        if (newPlainPassword.size() < 4) {
            throw Exceptions::ValidationException("Новый пароль", "Пароль должен содержать минимум 4 символа.");
        }
        salt_ = Security::PasswordHasher::generateSalt(16);
        passwordHash_ = Security::PasswordHasher::hashPassword(newPlainPassword, salt_);
    }

    std::string User::getRoleDisplayName() const {
        switch (role_) {
        case UserRole::ADMIN:
            return "Администратор";
        case UserRole::PHARMACIST:
            return "Фармацевт";
        default:
            return "Неизвестно";
        }
    }

    std::string User::roleToString(UserRole role) {
        return (role == UserRole::ADMIN) ? "ADMIN" : "PHARMACIST";
    }

    UserRole User::stringToRole(const std::string& str) {
        if (str == "ADMIN") return UserRole::ADMIN;
        if (str == "PHARMACIST") return UserRole::PHARMACIST;
        throw Exceptions::ValidationException("Роль пользователя", "Неизвестный код роли: " + str);
    }

    bool User::operator==(const User& other) const {
        return username_ == other.username_;
    }

    std::ostream& operator<<(std::ostream& os, const User& user) {
        os << user.username_ << "|"
            << user.salt_ << "|"
            << user.passwordHash_ << "|"
            << User::roleToString(user.role_);
        return os;
    }

    std::istream& operator>>(std::istream& is, User& user) {
        std::string line;
        if (!std::getline(is, line)) {
            return is;
        }

        line = Utils::ConsoleUtils::trim(line);
        if (line.empty()) {
            is.setstate(std::ios::failbit);
            return is;
        }

        std::vector<std::string> parts = Utils::ConsoleUtils::split(line, '|');
        if (parts.size() != 4) {
            is.setstate(std::ios::failbit);
            return is;
        }

        user.username_ = parts[0];
        user.salt_ = parts[1];
        user.passwordHash_ = parts[2];
        try {
            user.role_ = User::stringToRole(parts[3]);
        }
        catch (...) {
            is.setstate(std::ios::failbit);
        }

        return is;
    }

} // namespace PharmacySystem