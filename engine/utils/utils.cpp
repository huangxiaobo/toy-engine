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

// 定义 STB_IMAGE_IMPLEMENTATION 以包含 stb_image 的实现
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/gl.h>


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

unsigned int Utils::LoadTextureFromFile(const std::string &path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // stbi_load 会自动翻转Y轴，使图像原点位于左下角（OpenGL坐标系）
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            format = GL_RGB; // 默认

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 设置纹理环绕和过滤参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int Utils::CreateCheckerboardTexture(int width, int height, int checkSize) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // 创建棋盘格数据 (RGBA)
    std::vector<unsigned char> data(width * height * 4); // RGBA
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 4;
            
            // 计算棋盘格颜色
            int xCheck = (x / checkSize) % 2;
            int yCheck = (y / checkSize) % 2;
            
            if (xCheck == yCheck) {
                // 白色方格
                data[index] = 255;     // R
                data[index + 1] = 255; // G
                data[index + 2] = 255; // B
                data[index + 3] = 255; // A
            } else {
                // 黑色方格
                data[index] = 0;       // R
                data[index + 1] = 0;   // G
                data[index + 2] = 0;   // B
                data[index + 3] = 255; // A
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    // 设置纹理环绕和过滤参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}
