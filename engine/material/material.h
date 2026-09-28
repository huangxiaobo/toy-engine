#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <glm/glm.hpp>

#include <string>

class Material {
public:
    Material();

    ~Material();

    std::string m_id; // 材质唯一标识符（UUID，构造时自动生成）
    std::string m_name; // 材质名称（对应 MTL 中的 newmtl <name>）
    // 逐绘制状态：材质随 Model 走，绘制前由 Mesh::Draw 上传，Technique 不持有
    glm::vec3 m_ambient_color = glm::vec3(0.0f); // 环境
    glm::vec3 m_diffuse_color = glm::vec3(0.0f); // 漫反射
    glm::vec3 m_specular_color = glm::vec3(0.0f); // 镜面反射
    glm::f32 m_shininess = 0.0f; // 镜面反射光泽
};

#endif
