#include "Order.h"
#include "ConsoleUtils.h"
#include "Exceptions.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace PharmacySystem {

    int Order::nextOrderId_ = 1001;
    double Order::totalAccumulatedRevenue_ = 0.0;

    Order::Order(std::string pharmacistUsername)
        : orderId_(nextOrderId_++),
        timestamp_(Utils::ConsoleUtils::getCurrentDateTime()),
        pharmacistUsername_(std::move(pharmacistUsername)),
        prescriptionDetails_("Не требуется"),
        totalAmount_(0.0),
        isFinalized_(false) {
    }

    Order::Order(int orderId,
        std::string timestamp,
        std::string pharmacistUsername,
        std::string prescriptionDetails,
        double totalAmount,
        std::vector<OrderItem> items)
        : orderId_(orderId),
        timestamp_(std::move(timestamp)),
        pharmacistUsername_(std::move(pharmacistUsername)),
        prescriptionDetails_(std::move(prescriptionDetails)),
        items_(std::move(items)),
        totalAmount_(totalAmount),
        isFinalized_(true) {
        syncNextOrderId(orderId_);
    }

    void Order::addMedicineItem(const std::shared_ptr<Medicine>& medicine, int quantity) {
        if (!medicine) {
            throw Exceptions::ValidationException("Позиция заказа", "Передан пустой указатель на препарат.");
        }
        if (isFinalized_) {
            throw Exceptions::ValidationException("Заказ", "Нельзя изменять уже проведённый заказ.");
        }
        if (quantity <= 0) {
            throw Exceptions::ValidationException("Количество", "Количество должно быть больше нуля.");
        }
        if (medicine->isExpired()) {
            throw Exceptions::OutOfStockException("Препарат '" + medicine->getName() +
                "' просрочен (срок годности: " + medicine->getExpiryDate() + ").");
        }

        // Проверяем суммарное количество с учётом уже добавленных позиций этого препарата в корзине
        int alreadyInCart = 0;
        for (const auto& existing : items_) {
            if (existing.medicineId == medicine->getId()) {
                alreadyInCart += existing.quantity;
            }
        }

        if (alreadyInCart + quantity > medicine->getStockQuantity()) {
            throw Exceptions::OutOfStockException(medicine->getName(),
                alreadyInCart + quantity,
                medicine->getStockQuantity());
        }

        // Если препарат уже есть в корзине — объединяем количество и пересчитываем стоимость
        for (auto& existing : items_) {
            if (existing.medicineId == medicine->getId()) {
                existing.quantity += quantity;
                existing.lineTotalPrice = medicine->calculateFinalPrice(existing.quantity);
                totalAmount_ = 0.0;
                for (const auto& it : items_) {
                    totalAmount_ += it.lineTotalPrice;
                }
                return;
            }
        }

        OrderItem newItem{
            medicine->getId(),
            medicine->getName(),
            medicine->getCategoryName(),
            quantity,
            medicine->getPrice(),
            medicine->calculateFinalPrice(quantity),
            medicine->requiresPrescription()
        };

        *this += newItem;
    }

    Order& Order::operator+=(const OrderItem& item) {
        if (isFinalized_) {
            throw Exceptions::ValidationException("Заказ", "Нельзя добавлять позиции в закрытый чек.");
        }
        items_.push_back(item);
        totalAmount_ += item.lineTotalPrice;
        return *this;
    }

    bool Order::hasPrescriptionItems() const noexcept {
        for (const auto& item : items_) {
            if (item.requiresRx) {
                return true;
            }
        }
        return false;
    }

    void Order::setPrescriptionDetails(const std::string& details) {
        if (hasPrescriptionItems() && Utils::ConsoleUtils::trim(details).empty()) {
            throw Exceptions::ValidationException("Реквизиты рецепта",
                "Для отпуска рецептурных/ПКУ препаратов обязательны серия и номер рецепта.");
        }
        prescriptionDetails_ = Utils::ConsoleUtils::trim(details);
    }

    void Order::finalizeOrder(std::vector<std::shared_ptr<Medicine>>& catalog) {
        if (isFinalized_) {
            throw Exceptions::ValidationException("Проведение заказа", "Заказ №" + std::to_string(orderId_) + " уже проведён.");
        }
        if (items_.empty()) {
            throw Exceptions::ValidationException("Проведение заказа", "Невозможно оформить пустой заказ.");
        }
        if (hasPrescriptionItems() &&
            (prescriptionDetails_.empty() || prescriptionDetails_ == "Не требуется")) {
            throw Exceptions::ValidationException("Контроль рецепта",
                "Не указаны реквизиты рецепта для рецептурных препаратов.");
        }

        // 1. Предварительная проверка наличия всех позиций (атомарность операции)
        for (const auto& item : items_) {
            bool found = false;
            for (const auto& med : catalog) {
                if (med->getId() == item.medicineId) {
                    found = true;
                    if (med->isExpired()) {
                        throw Exceptions::OutOfStockException("Препарат '" + med->getName() + "' просрочен.");
                    }
                    if (med->getStockQuantity() < item.quantity) {
                        throw Exceptions::OutOfStockException(med->getName(), item.quantity, med->getStockQuantity());
                    }
                    break;
                }
            }
            if (!found) {
                throw Exceptions::ValidationException("Каталог", "Препарат с ID " + std::to_string(item.medicineId) + " не найден.");
            }
        }

        // 2. Фактическое списание остатков со склада
        for (const auto& item : items_) {
            for (auto& med : catalog) {
                if (med->getId() == item.medicineId) {
                    med->reduceStock(item.quantity);
                    break;
                }
            }
        }

        timestamp_ = Utils::ConsoleUtils::getCurrentDateTime();
        isFinalized_ = true;
        totalAccumulatedRevenue_ += totalAmount_;
    }

    std::string Order::generateReceiptText() const {
        std::ostringstream oss;
        const int width = 82;
        std::string lineEq(width, '=');
        std::string lineDash(width, '-');

        oss << lineEq << "\n"
            << "                 АИС АПТЕКИ — КАССОВЫЙ ЧЕК / НАКЛАДНАЯ ОТПУСКА\n"
            << lineEq << "\n"
            << "Номер документа:    № " << orderId_ << "\n"
            << "Дата и время:       " << timestamp_ << "\n"
            << "Сотрудник (кассир): " << pharmacistUsername_ << "\n"
            << "Рецепт врача:       " << prescriptionDetails_ << "\n"
            << lineDash << "\n"
            << "| " << Utils::ConsoleUtils::padRightUTF8("ID", 4)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Наименование препарата", 24)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Категория", 15)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Кол-во", 6)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Цена", 8)
            << " | " << Utils::ConsoleUtils::padRightUTF8("Сумма", 10) << " |\n"
            << lineDash << "\n";

        for (const auto& item : items_) {
            std::ostringstream unitPriceSs, totalSs;
            unitPriceSs << std::fixed << std::setprecision(2) << item.unitBasePrice;
            totalSs << std::fixed << std::setprecision(2) << item.lineTotalPrice;

            oss << "| " << Utils::ConsoleUtils::padRightUTF8(std::to_string(item.medicineId), 4)
                << " | " << Utils::ConsoleUtils::padRightUTF8(item.medicineName, 24)
                << " | " << Utils::ConsoleUtils::padRightUTF8(item.categoryName, 15)
                << " | " << Utils::ConsoleUtils::padRightUTF8(std::to_string(item.quantity), 6)
                << " | " << Utils::ConsoleUtils::padRightUTF8(unitPriceSs.str(), 8)
                << " | " << Utils::ConsoleUtils::padRightUTF8(totalSs.str(), 10) << " |\n";
        }

        oss << lineDash << "\n"
            << "ИТОГО К ОПЛАТЕ: " << std::fixed << std::setprecision(2) << totalAmount_ << " руб.\n"
            << "Статус документа: " << (isFinalized_ ? "ПРОВЕДЁН (Списано со склада)" : "ЧЕРНОВИК (В корзине)") << "\n"
            << lineEq << "\n";

        return oss.str();
    }

    bool Order::exportReceiptToTxt(const std::string& filePath) const {
        std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
        if (!outFile.is_open() || !outFile.good()) {
            throw Exceptions::FileAccessException(filePath, "Не удалось создать файл отчёта/чека.");
        }
        outFile << generateReceiptText();
        outFile.flush();
        return outFile.good();
    }

    void Order::syncNextOrderId(int maxExistingId) noexcept {
        if (maxExistingId >= nextOrderId_) {
            nextOrderId_ = maxExistingId + 1;
        }
    }

    void Order::addToTotalRevenue(double amount) noexcept {
        if (amount > 0.0) {
            totalAccumulatedRevenue_ += amount;
        }
    }

    void Order::resetTotalRevenue() noexcept {
        totalAccumulatedRevenue_ = 0.0;
    }

    double Order::getTotalAccumulatedRevenue() noexcept {
        return totalAccumulatedRevenue_;
    }

    std::string Order::serialize() const {
        // Формат: ORDER_ID|TIMESTAMP|PHARMACIST|RX_DETAILS|TOTAL_AMOUNT|ITEM_COUNT|id~name~cat~qty~unit~line~rx;...
        std::ostringstream oss;
        oss << orderId_ << "|"
            << timestamp_ << "|"
            << pharmacistUsername_ << "|"
            << prescriptionDetails_ << "|"
            << std::fixed << std::setprecision(2) << totalAmount_ << "|"
            << items_.size() << "|";

        for (size_t i = 0; i < items_.size(); ++i) {
            const auto& it = items_[i];
            oss << it.medicineId << "~"
                << it.medicineName << "~"
                << it.categoryName << "~"
                << it.quantity << "~"
                << std::fixed << std::setprecision(2) << it.unitBasePrice << "~"
                << std::fixed << std::setprecision(2) << it.lineTotalPrice << "~"
                << (it.requiresRx ? "1" : "0");
            if (i + 1 < items_.size()) {
                oss << ";";
            }
        }
        return oss.str();
    }

    Order Order::deserialize(const std::string& line) {
        std::string trimmed = Utils::ConsoleUtils::trim(line);
        std::vector<std::string> parts = Utils::ConsoleUtils::split(trimmed, '|');
        if (parts.size() < 7) {
            throw Exceptions::ValidationException("Десериализация заказа", "Некорректная структура строки заказа.");
        }

        int id = std::stoi(parts[0]);
        const std::string& ts = parts[1];
        const std::string& user = parts[2];
        const std::string& rx = parts[3];
        double total = std::stod(parts[4]);
        size_t expectedItemCount = static_cast<size_t>(std::stoul(parts[5]));

        std::vector<OrderItem> parsedItems;
        if (!parts[6].empty()) {
            std::vector<std::string> itemTokens = Utils::ConsoleUtils::split(parts[6], ';');
            for (const auto& token : itemTokens) {
                if (token.empty()) continue;
                std::vector<std::string> fields = Utils::ConsoleUtils::split(token, '~');
                if (fields.size() != 7) {
                    throw Exceptions::ValidationException("Десериализация позиции заказа",
                        "Неверное число полей позиции: " + token);
                }
                OrderItem item{
                    std::stoi(fields[0]),
                    fields[1],
                    fields[2],
                    std::stoi(fields[3]),
                    std::stod(fields[4]),
                    std::stod(fields[5]),
                    (fields[6] == "1")
                };
                parsedItems.push_back(std::move(item));
            }
        }

        if (parsedItems.size() != expectedItemCount) {
            throw Exceptions::ValidationException("Десериализация заказа",
                "Несовпадение числа позиций в строке заказа №" + std::to_string(id));
        }

        return Order(id, ts, user, rx, total, std::move(parsedItems));
    }

    std::ostream& operator<<(std::ostream& os, const Order& order) {
        os << order.generateReceiptText();
        return os;
    }

} // namespace PharmacySystem