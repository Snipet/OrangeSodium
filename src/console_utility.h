#pragma once

// Utility functions for console

#include <iostream>

namespace OrangeSodium {

namespace ConsoleUtility {

inline void logGreen(std::ostream* log_stream, const std::string& message) {
    if(log_stream == &std::cout) {
        *log_stream << "\033[32m" << message << "\033[0m" << std::endl;
        return;
    }
    *log_stream << message << std::endl;
}

inline void logRed(std::ostream* log_stream, const std::string& message) {
    if(log_stream == &std::cout) {
        *log_stream << "\033[31m" << message << "\033[0m" << std::endl;
        return;
    }
    *log_stream << message << std::endl;
}
} // namespace ConsoleUtility
} // namespace OrangeSodium