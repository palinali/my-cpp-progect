#pragma once
#ifndef PHARMACY_REPOSITORY_H
#define PHARMACY_REPOSITORY_H

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace Storage {

    /**
     * @brief Шаблонный класс-репозиторий для управления коллекцией объектов в памяти.
     * Оптимизирован для работы с полиморфными умными указателями (например, std::shared_ptr<Medicine>)
     * и произвольными объектами моделей (User, Order) поверх контейнера std::vector<T>.
     */
    template <typename T>
    class Repository {
    private:
        std::vector<T> items_;

    public:
        Repository() = default;
        ~Repository() = default;

        // Добавление элемента в хранилище
        void add(const T& item) {
            items_.push_back(item);
        }

        void add(T&& item) {
            items_.push_back(std::move(item));
        }

        // Удаление элементов по предикату; возвращает количество удалённых записей
        size_t removeIf(const std::function<bool(const T&)>& predicate) {
            size_t oldSize = items_.size();
            auto newEnd = std::remove_if(items_.begin(), items_.end(), predicate);
            items_.erase(newEnd, items_.end());
            return oldSize - items_.size();
        }

        // Поиск первого элемента, удовлетворяющего условию (возвращает nullptr, если не найден)
        T* findFirst(const std::function<bool(const T&)>& predicate) {
            auto it = std::find_if(items_.begin(), items_.end(), predicate);
            if (it != items_.end()) {
                return &(*it);
            }
            return nullptr;
        }

        const T* findFirst(const std::function<bool(const T&)>& predicate) const {
            auto it = std::find_if(items_.begin(), items_.end(), predicate);
            if (it != items_.end()) {
                return &(*it);
            }
            return nullptr;
        }

        // Фильтрация всех элементов по предикату
        std::vector<T> findAll(const std::function<bool(const T&)>& predicate) const {
            std::vector<T> result;
            for (const auto& item : items_) {
                if (predicate(item)) {
                    result.push_back(item);
                }
            }
            return result;
        }

        // Сортировка элементов на месте с использованием пользовательского компаратора
        void sortBy(const std::function<bool(const T&, const T&)>& comparator) {
            std::sort(items_.begin(), items_.end(), comparator);
        }

        // Доступ к базовому контейнеру
        inline std::vector<T>& getAll() noexcept { return items_; }
        inline const std::vector<T>& getAll() const noexcept { return items_; }

        inline size_t size() const noexcept { return items_.size(); }
        inline bool empty() const noexcept { return items_.empty(); }
        inline void clear() noexcept { items_.clear(); }

        T& at(size_t index) {
            if (index >= items_.size()) {
                throw std::out_of_range("Индекс выходит за пределы репозитория.");
            }
            return items_[index];
        }

        const T& at(size_t index) const {
            if (index >= items_.size()) {
                throw std::out_of_range("Индекс выходит за пределы репозитория.");
            }
            return items_[index];
        }
    };

} // namespace Storage

#endif // PHARMACY_REPOSITORY_H