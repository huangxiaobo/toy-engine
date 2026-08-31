#include "mtl_parser.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include "material.h"

MtlParser::MtlParser() {
}

MtlParser::~MtlParser() {
}

// 解析一行形如 "Ka 0.25 0.25 0.25" 的颜色指令，返回 glm::vec3；解析失败返回空标记。
static bool ParseColor(std::istringstream &iss, glm::vec3 &out) {
    // 需要至少三个浮点数（r g b）
    float r, g, b;
    if (!(iss >> r >> g >> b)) {
        return false;
    }
    out = glm::vec3(r, g, b);
    return true;
}

std::vector<Material *> MtlParser::ParseFromFile(const std::string &filePath) {
    std::vector<Material *> materials;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[MtlParser] 打开 MTL 文件失败: " << filePath << std::endl;
        return materials;
    }

    // 当前正在写入的材质。遇到 newmtl 时创建新材质并加入列表。
    Material *current = nullptr;

    std::string line;
    while (std::getline(file, line)) {
        // 1. 去掉行首空白
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            // 空行，跳过
            continue;
        }
        // 2. 跳过注释（以 # 开头）
        if (line[start] == '#') {
            continue;
        }
        // 3. 用字符串流解析指令
        std::istringstream iss(line.substr(start));
        std::string key;
        if (!(iss >> key)) {
            continue;
        }

        if (key == "newmtl") {
            // 声明新材质：newmtl <material_name>
            // 材质名可能包含空格，因此取剩余整行作为名字
            std::string name;
            std::getline(iss, name);
            // 去掉名字首尾空白
            size_t n_start = name.find_first_not_of(" \t");
            size_t n_end = name.find_last_not_of(" \t");
            if (n_start == std::string::npos) {
                // 空材质名，忽略
                current = nullptr;
                continue;
            }
            name = name.substr(n_start, n_end - n_start + 1);

            current = new Material();
            current->Name = name;
            // 默认值：与 OpenGL 常见默认一致，避免缺字段时出现未定义值
            current->AmbientColor = glm::vec3(0.2f);
            current->DiffuseColor = glm::vec3(0.8f);
            current->SpecularColor = glm::vec3(0.0f);
            current->Shininess = 0.0f;
            materials.push_back(current);
        } else if (current == nullptr) {
            // 尚未有 newmtl 声明的字段，忽略（非规范写法）
            continue;
        } else if (key == "Ka") {
            glm::vec3 c;
            if (ParseColor(iss, c)) {
                current->AmbientColor = c;
            }
        } else if (key == "Kd") {
            glm::vec3 c;
            if (ParseColor(iss, c)) {
                current->DiffuseColor = c;
            }
        } else if (key == "Ks") {
            glm::vec3 c;
            if (ParseColor(iss, c)) {
                current->SpecularColor = c;
            }
        } else if (key == "Ns") {
            float s;
            if (iss >> s) {
                current->Shininess = s;
            }
        }
        // 其他 MTL 指令（map_Kd、d、Tr、illum 等）当前引擎不支持，直接忽略
    }

    file.close();
    return materials;
}

Material *MtlParser::ParseSingle(const std::string &filePath, const std::string &materialName) {
    // 先解析整个文件
    std::vector<Material *> materials = ParseFromFile(filePath);

    // 按 newmtl 声明的名称精确匹配；找不到则返回 nullptr
    Material *result = nullptr;
    for (auto *mat : materials) {
        if (mat->Name == materialName) {
            result = mat;
            break;
        }
    }
    // 释放所有未被选中的材质，仅保留结果，避免内存泄漏
    for (auto *mat : materials) {
        if (mat != result) {
            delete mat;
        }
    }
    return result;
}
