#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <glm/glm.hpp>

class Material
{
public:
    Material();
    ~Material();

    glm::vec3 AmbientColor;  // 环境
    glm::vec3 DiffuseColor;  // 漫反射
    glm::vec3 SpecularColor; // 镜面反射
    glm::f32 Shininess;      // 镜面反射光泽
};

#endif