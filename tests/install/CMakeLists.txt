# Checks that an installed Radon is usable: cmake -S tests/install -B out -DCMAKE_PREFIX_PATH=<prefix>
cmake_minimum_required(VERSION 3.20)
project(radon_install_check LANGUAGES CXX)
find_package(Radon 0.2 REQUIRED)
add_executable(radon_install_check main.cpp)
target_link_libraries(radon_install_check PRIVATE radon::radon)
