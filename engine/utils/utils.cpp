#include "utils.h"

#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

void Utils::DebugMatrix(const glm::mat4 &mat)
{
    std::cout << "[" << std::endl;

    for (int i = 0; i < 4; i++)
    {
        std::cout << "  [ " << mat[i][0] << "," << mat[i][1] << "," << mat[i][2] << "," << mat[i][3] << " ]" << std::endl;
    }
    std::cout << "]" << std::endl;
}

glm::vec3 Utils::GetXYZ(tinyxml2::XMLElement *element)
{
    if (element != nullptr)
    {
        auto x = element->FirstChildElement("x")->FloatText();
        auto y = element->FirstChildElement("y")->FloatText();
        auto z = element->FirstChildElement("z")->FloatText();
        return glm::vec3(x, y, z);
    }
    return glm::vec3(0.0f);
}

std::string Utils::GetString(const glm::vec3 vec)
{
    return "(" + std::to_string(vec.x) + "," + std::to_string(vec.y) + "," + std::to_string(vec.z) + ")";
}

std::string Utils::GetString(const glm::vec2 vec)
{
    return "(" + std::to_string(vec.x) + "," + std::to_string(vec.y) + ")";
}

glm::vec3 Utils::GetRGB(tinyxml2::XMLElement *element)
{

    if (element != nullptr)
    {
        auto r = element->FirstChildElement("r")->FloatText();
        auto g = element->FirstChildElement("g")->FloatText();
        auto b = element->FirstChildElement("b")->FloatText();
        return glm::vec3(r, g, b);
    }
    return glm::vec3(0.0f);
}

std::string Utils::GenerateUUID()
{
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
