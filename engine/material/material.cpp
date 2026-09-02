#include "material.h"
#include "../utils/utils.h"

/*
 * 材质（Material）：描述物体表面对光的响应参数
 *
 * 纯数据类：AmbientColor/DiffuseColor/SpecularColor/Shininess
 * 由 TechniqueLight::SetMaterial() 同步到着色器 gMaterial uniform。
 * 成员默认值定义于 material.h 中。
 */
Material::Material() {
    // 构造时自动生成唯一 UUID
    Id = Utils::GenerateUUID();
}

Material::~Material() {
}