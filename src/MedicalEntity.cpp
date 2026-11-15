#include "MedicalEntity.h"
#include "Exceptions.h"
#include <utility>

namespace PharmacySystem {

    int MedicalEntity::nextId_ = 1;
    int MedicalEntity::activeObjectCount_ = 0;

    MedicalEntity::MedicalEntity(int id, std::string name)
        : id_(id), name_(std::move(name)) {
        if (name_.empty()) {
            throw Exceptions::ValidationException("Наименование", "Наименование не может быть пустым.");
        }
        if (id_ >= nextId_) {
            nextId_ = id_ + 1;
        }
        ++activeObjectCount_;
    }

    MedicalEntity::~MedicalEntity() noexcept {
        --activeObjectCount_;
    }

    void MedicalEntity::setName(const std::string& name) {
        if (name.empty()) {
            throw Exceptions::ValidationException("Наименование", "Наименование не может быть пустым.");
        }
        name_ = name;
    }

    int MedicalEntity::generateNextId() noexcept {
        return nextId_++;
    }

    void MedicalEntity::syncNextId(int maxExistingId) noexcept {
        if (maxExistingId >= nextId_) {
            nextId_ = maxExistingId + 1;
        }
    }

    int MedicalEntity::getActiveObjectCount() noexcept {
        return activeObjectCount_;
    }

} // namespace PharmacySystem