cmake_minimum_required(VERSION 3.16)
project(PharmacyAIS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8 /W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

include_directories(include)

set(SOURCES
    src/main.cpp
    src/Exceptions.cpp
    src/ConsoleUtils.cpp
    src/Security.cpp
    src/User.cpp
    src/MedicalEntity.cpp
    src/Medicine.cpp
    src/DerivedMedicines.cpp
    src/MedicineFactory.cpp
    src/Order.cpp
    src/FileManager.cpp
    src/Menu.cpp
)

add_executable(pharmacy_ais ${SOURCES})