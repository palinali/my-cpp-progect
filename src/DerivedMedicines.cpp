#include "DerivedMedicines.h"
#include "Exceptions.h"
#include <iomanip>
#include <sstream>
#include <utility>

namespace PharmacySystem {

    // ==================== OTCMedicine ====================

    OTCMedicine::OTCMedicine(int id,
        std::string name,
        std::string activeIngredient,
        std::string manufacturer,
        double price,
        int stockQuantity,
        std::string expiryDate,
        std::string pharmacologicalGroup)
        : Medicine(id,
            std::move(name),
            std::move(activeIngredient),
            std::move(manufacturer),
            price,
            stockQuantity,
            std::move(expiryDate)),
        pharmacologicalGroup_(pharmacologicalGroup.empty() ? "Общая терапия" : std::move(pharmacologicalGroup)) {
    }

    void OTCMedicine::setPharmacologicalGroup(const std::string& group) {
        if (group.empty()) {
            throw Exceptions::ValidationException("Фарм. группа", "Фармакологическая группа не может быть пустой.");
        }
        pharmacologicalGroup_ = group;
    }

    std::string OTCMedicine::getCategoryName() const {
        return "Безрецептурный";
    }

    std::string OTCMedicine::getDispenseRule() const {
        return "Свободный отпуск без рецепта (Группа: " + pharmacologicalGroup_ + ")";
    }

    std::string OTCMedicine::serialize() const {
        return Medicine::serialize() + "|" + pharmacologicalGroup_;
    }

    void OTCMedicine::printDetails(std::ostream& os) const {
        Medicine::printDetails(os);
        os << "  Фарм. группа:           " << pharmacologicalGroup_ << "\n";
    }

    // ==================== PrescriptionMedicine ====================

    PrescriptionMedicine::PrescriptionMedicine(int id,
        std::string name,
        std::string activeIngredient,
        std::string manufacturer,
        double price,
        int stockQuantity,
        std::string expiryDate,
        std::string prescriptionFormType)
        : Medicine(id,
            std::move(name),
            std::move(activeIngredient),
            std::move(manufacturer),
            price,
            stockQuantity,
            std::move(expiryDate)),
        prescriptionFormType_(prescriptionFormType.empty() ? "Форма 107-1/у" : std::move(prescriptionFormType)) {
    }

    void PrescriptionMedicine::setPrescriptionFormType(const std::string& formType) {
        if (formType.empty()) {
            throw Exceptions::ValidationException("Форма рецепта", "Тип рецептурного бланка не может быть пустым.");
        }
        prescriptionFormType_ = formType;
    }

    std::string PrescriptionMedicine::getCategoryName() const {
        return "Рецептурный";
    }

    std::string PrescriptionMedicine::getDispenseRule() const {
        return "Строго по рецепту врача (" + prescriptionFormType_ + ")";
    }

    std::string PrescriptionMedicine::serialize() const {
        return Medicine::serialize() + "|" + prescriptionFormType_;
    }

    void PrescriptionMedicine::printDetails(std::ostream& os) const {
        Medicine::printDetails(os);
        os << "  Рецептурный бланк:      " << prescriptionFormType_ << "\n";
    }

    // ==================== NarcoticControlMedicine ====================

    NarcoticControlMedicine::NarcoticControlMedicine(int id,
        std::string name,
        std::string activeIngredient,
        std::string manufacturer,
        double price,
        int stockQuantity,
        std::string expiryDate,
        std::string registryLicenseCode,
        double specialHandlingFeePercent)
        : Medicine(id,
            std::move(name),
            std::move(activeIngredient),
            std::move(manufacturer),
            price,
            stockQuantity,
            std::move(expiryDate)),
        registryLicenseCode_(registryLicenseCode.empty() ? "ПКУ-148/у-88" : std::move(registryLicenseCode)),
        specialHandlingFeePercent_(specialHandlingFeePercent) {
        if (specialHandlingFeePercent_ < 0.0 || specialHandlingFeePercent_ > 100.0) {
            throw Exceptions::ValidationException("Надбавка ПКУ", "Процент надбавки должен быть от 0 до 100.");
        }
    }

    void NarcoticControlMedicine::setRegistryLicenseCode(const std::string& code) {
        if (code.empty()) {
            throw Exceptions::ValidationException("Код реестра ПКУ", "Код спецучёта не может быть пустым.");
        }
        registryLicenseCode_ = code;
    }

    void NarcoticControlMedicine::setSpecialHandlingFeePercent(double percent) {
        if (percent < 0.0 || percent > 100.0) {
            throw Exceptions::ValidationException("Надбавка ПКУ", "Процент надбавки должен быть от 0 до 100.");
        }
        specialHandlingFeePercent_ = percent;
    }

    double NarcoticControlMedicine::calculateFinalPrice(int quantity) const {
        double baseTotal = Medicine::calculateFinalPrice(quantity);
        return baseTotal * (1.0 + specialHandlingFeePercent_ / 100.0);
    }

    std::string NarcoticControlMedicine::getCategoryName() const {
        return "ПКУ (Спецучёт)";
    }

    std::string NarcoticControlMedicine::getDispenseRule() const {
        std::ostringstream oss;
        oss << "Предметно-количественный учёт [" << registryLicenseCode_
            << "], надбавка спецучёта: +" << std::fixed << std::setprecision(1)
            << specialHandlingFeePercent_ << "%";
        return oss.str();
    }

    std::string NarcoticControlMedicine::serialize() const {
        std::ostringstream oss;
        oss << Medicine::serialize() << "|"
            << registryLicenseCode_ << "|"
            << std::fixed << std::setprecision(2) << specialHandlingFeePercent_;
        return oss.str();
    }

    void NarcoticControlMedicine::printDetails(std::ostream& os) const {
        Medicine::printDetails(os);
        os << "  Код реестра ПКУ:        " << registryLicenseCode_ << "\n"
            << "  Надбавка за спецучёт:   " << std::fixed << std::setprecision(1)
            << specialHandlingFeePercent_ << "%\n";
    }

} // namespace PharmacySystem