#include "program.h"
#include <fstream>

namespace OrangeSodium {
Program::Program(Context* context) : context(context), program_path(""), program_name("") {
}

bool Program::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    program_data = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return true;
}

} // namespace OrangeSodium