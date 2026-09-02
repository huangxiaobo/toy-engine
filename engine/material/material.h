#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <glm/glm.hpp>

#include <string>

class Material {
public:
    Material();

    ~Material();

    std::string Id; // 材质唯一标识符（UUID，构造时自动生成）
    std::string Name; // 材质名称（对应 MTL 中的 newmtl <name>）
    glm::vec3 AmbientColor; // 环境
    glm::vec3 DiffuseColor; // 漫反射
    glm::vec3 SpecularColor; // 镜面反射
    glm::f32 Shininess; // 镜面反射光泽
};

#endif
