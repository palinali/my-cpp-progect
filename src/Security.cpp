#include "Security.h"
#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>

namespace Security {

    // ==================== CryptoBuffer (Правило Пяти) ====================

    CryptoBuffer::CryptoBuffer(size_t size) : data_(nullptr), size_(size) {
        if (size_ > 0) {
            data_ = new uint8_t[size_]{};
        }
    }

    CryptoBuffer::CryptoBuffer(const std::string& text) : data_(nullptr), size_(text.size()) {
        if (size_ > 0) {
            data_ = new uint8_t[size_];
            std::memcpy(data_, text.data(), size_);
        }
    }

    CryptoBuffer::~CryptoBuffer() {
        if (data_ != nullptr) {
            // Затираем чувствительные данные в памяти перед освобождением
            std::memset(data_, 0, size_);
            delete[] data_;
            data_ = nullptr;
        }
        size_ = 0;
    }

    CryptoBuffer::CryptoBuffer(const CryptoBuffer& other) : data_(nullptr), size_(other.size_) {
        if (size_ > 0 && other.data_ != nullptr) {
            data_ = new uint8_t[size_];
            std::memcpy(data_, other.data_, size_);
        }
    }

    CryptoBuffer& CryptoBuffer::operator=(const CryptoBuffer& other) {
        if (this == &other) {
            return *this;
        }

        // Освобождаем текущую память
        if (data_ != nullptr) {
            std::memset(data_, 0, size_);
            delete[] data_;
            data_ = nullptr;
        }

        size_ = other.size_;
        if (size_ > 0 && other.data_ != nullptr) {
            data_ = new uint8_t[size_];
            std::memcpy(data_, other.data_, size_);
        }

        return *this;
    }

    CryptoBuffer::CryptoBuffer(CryptoBuffer&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    CryptoBuffer& CryptoBuffer::operator=(CryptoBuffer&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        if (data_ != nullptr) {
            std::memset(data_, 0, size_);
            delete[] data_;
        }

        data_ = other.data_;
        size_ = other.size_;

        other.data_ = nullptr;
        other.size_ = 0;

        return *this;
    }

    void CryptoBuffer::applyXorWithRotation(uint8_t baseKey) {
        if (data_ == nullptr || size_ == 0) return;

        for (size_t i = 0; i < size_; ++i) {
            uint8_t dynamicKey = static_cast<uint8_t>(baseKey ^ ((i * 31 + 17) & 0xFF));
            uint8_t val = data_[i] ^ dynamicKey;
            // Циклический битовый сдвиг влево на 3 бита
            data_[i] = static_cast<uint8_t>((val << 3) | (val >> 5));
        }
    }

    std::string CryptoBuffer::toHexString() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < size_; ++i) {
            oss << std::setw(2) << static_cast<int>(data_[i]);
        }
        return oss.str();
    }

    // ==================== PasswordHasher ====================

    std::string PasswordHasher::generateSalt(size_t length) {
        static const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);

        std::string salt;
        salt.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            salt.push_back(charset[dist(generator)]);
        }
        return salt;
    }

    std::string PasswordHasher::hashPassword(const std::string& password, const std::string& salt) {
        std::string combined = salt + ":" + password + ":PHARMACY_AIS_2026";
        CryptoBuffer buffer(combined);

        // Несколько раундов перемешивания с циклическим сдвигом и XOR
        buffer.applyXorWithRotation(0x5A);
        buffer.applyXorWithRotation(0xA5);

        // Вычисляем 256-битную (4 x 64-бит) полиномиальную свёртку поверх преобразованного буфера
        uint64_t h1 = 0xCBF29CE484222325ULL;
        uint64_t h2 = 0x100000001B3ULL;
        uint64_t h3 = 0x9E3779B97F4A7C15ULL;
        uint64_t h4 = 0x517CC1B727220A95ULL;

        const uint8_t* bytes = buffer.rawData();
        for (size_t i = 0; i < buffer.size(); ++i) {
            uint64_t b = bytes[i];
            h1 = (h1 ^ b) * 1099511628211ULL;
            h2 = ((h2 << 7) | (h2 >> 57)) ^ (b * 1315423911ULL);
            h3 = (h3 + (b << (i % 24))) * 0xBF58476D1CE4E5B9ULL;
            h4 ^= (h1 >> 13) + (h2 << 11) + (b * 2654435761ULL);
        }

        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(16) << h1
            << std::setw(16) << h2
            << std::setw(16) << h3
            << std::setw(16) << h4;

        return oss.str();
    }

    bool PasswordHasher::verifyPassword(const std::string& password,
        const std::string& salt,
        const std::string& expectedHash) {
        return hashPassword(password, salt) == expectedHash;
    }

} // namespace Security