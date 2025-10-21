#pragma once

// Utility functions for console

#include <iostream>

namespace OrangeSodium {

namespace ConsoleUtility {

inline void logGreen(const std::string& message) {
    std::cout << "\033[32m" << message << "\033[0m" << std::endl;
}

inline void logRed(const std::string& message) {
    std::cout << "\033[31m" << message << "\033[0m" << std::endl;
}
} // namespace ConsoleUtility
} // namespace OrangeSodium