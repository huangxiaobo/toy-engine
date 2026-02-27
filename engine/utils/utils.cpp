#include "utils.h"

#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

#include <iostream>
#include <stdexcept>
#include <exception>
#include <execinfo.h>
#include <cxxabi.h>
#include <cstdlib>


void Utils::DebugMatrix(const glm::mat4 &mat) {
    std::cout << "[" << std::endl;

    for (int i = 0; i < 4; i++) {
        std::cout << "  [ " << mat[i][0] << "," << mat[i][1] << "," << mat[i][2] << "," << mat[i][3] << " ]" <<
                std::endl;
    }
    std::cout << "]" << std::endl;
}

std::string Utils::GetString(const glm::vec3 vec) {
    return "(" + std::to_string(vec.x) + "," + std::to_string(vec.y) + "," + std::to_string(vec.z) + ")";
}

std::string Utils::GetString(const glm::vec2 vec) {
    return "(" + std::to_string(vec.x) + "," + std::to_string(vec.y) + ")";
}

std::string Utils::GenerateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char hex_chars[] = "0123456789abcdef";

    // UUID format: 8-4-4-4-12 characters
    std::stringstream ss;
    for (int i = 0; i < 8; ++i)
        ss << hex_chars[dis(gen)];
    ss << "-";
    for (int i = 0; i < 4; ++i)
        ss << hex_chars[dis(gen)];
    ss << "-4"; // UUID version 4
    for (int i = 0; i < 3; ++i)
        ss << hex_chars[dis(gen)];
    ss << "-";
    ss << hex_chars[dis(gen) % 4 + 8]; // Variant bits
    for (int i = 0; i < 3; ++i)
        ss << hex_chars[dis(gen)];
    ss << "-";
    for (int i = 0; i < 12; ++i)
        ss << hex_chars[dis(gen)];

    return ss.str();
}


void Utils::PrintStackTrace() {
    const int max_frames = 64;
    void *stack_addrs[max_frames];
    int addr_count = backtrace(stack_addrs, max_frames);
    char **symbols = backtrace_symbols(stack_addrs, addr_count);

    std::cerr << "Stack trace:\n";

    for (int i = 0; i < addr_count; ++i) {
        char *begin_name = nullptr;
        char *end_name = nullptr;

        // 尝试提取函数名并解码
        for (char *p = symbols[i]; *p; ++p) {
            if (*p == '(') begin_name = p + 1;
            else if (*p == '+') end_name = p;
        }

        if (begin_name && end_name && begin_name < end_name) {
            *end_name = '\0';
            int status;
            char *demangled = abi::__cxa_demangle(begin_name, nullptr, nullptr, &status);
            if (demangled) {
                std::cerr << "  [" << i << "] " << demangled << "\n";
                free(demangled);
            } else {
                std::cerr << "  [" << i << "] " << begin_name << "\n";
            }
        } else {
            std::cerr << "  [" << i << "] " << symbols[i] << "\n";
        }
    }

    free(symbols);
}
