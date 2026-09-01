#include "technique_light.h"

#include "../light/light.h"
#include "../shader/shader.h"
#include "../material/material.h"
#include "../utils/utils.h"

#include <iostream>
#include <format>

/*
 * TechniqueLight 构造函数
 *
 * 扩展父类 Technique（着色器加载/编译/链接），专门处理带光照与材质的渲染：
 *   1. 设置类型为 TechniqueTypeLight，供渲染器区分（如地形 chunk 判断 shader 是否支持光照）
 *   2. 预取最多 8 个点光源的 uniform 位置（缓存 GLuint，避免每帧 GetUniformLocation 字符串查找开销）
 *   3. 初始化材质 uniform（gMaterial.*）
 *
 * 参数：
 *   name           - 技术名称，用于日志和调试
 *   vertexShader   - 顶点着色器文件路径
 *   fragmentShader - 片段着色器文件路径
 */
TechniqueLight::TechniqueLight(string name, string vertexShader, string fragmentShader)
    : Technique(name, vertexShader, fragmentShader) {
    m_type = TechniqueTypeLight;

    InitPointLightUniform(8);
    MaterialUniform.Init(this->m_shader);
}

TechniqueLight::~TechniqueLight() {
}

/*
 * 批量设置场景中的全部灯光
 *
 * 遍历光线列表，按类型分发：
 *   - 方向光 -> SetDirectionLight（直接填充基类 uniform）
 *   - 点光源 -> SetPointLight（写入点光源数组 uniform，并自动递增计数器）
 *   - 聚光灯 -> 当前未实现，跳过
 *
 * 注意：此方法在每帧绘制前由 Renderer 调用（如地形/模型 Draw 前 SetLights），
 * 将 CPU 侧灯光数据同步到 GPU uniform。
 */
void TechniqueLight::SetLights(const vector<Light *> &lights) {
    int point_light_count = 0;
    for (auto light: lights) {
        switch (light->GetLightType()) {
            case LightTypeDirection:
                SetDirectionLight((DirectionLight *) light);
                break;
            case LightTypePoint:
                SetPointLight(point_light_count++, (PointLight *) light);
                break;
            case LightTypeSpot:
                break;
            default:
                break;
        }
    }
}

/*
 * 设置方向光（平行光）的 uniform
 *
 * 方向光只有颜色/环境/漫反射/镜面反射四个分量，无位置与衰减，
 * 着色器中表现为全局光照参数（对应单个 uniform，非数组）。
 */
void TechniqueLight::SetDirectionLight(DirectionLight *light) {
    this->m_shader->SetUniformValue(LightColorUniform, light->Color);
    this->m_shader->SetUniformValue(LightAmbientUniform, light->AmbientColor);
    this->m_shader->SetUniformValue(LightDiffuseUniform, light->DiffuseColor);
    this->m_shader->SetUniformValue(LightSpecularUniform, light->SpecularColor);
}

/*
 * 预取点光源数组 uniform 位置
 *
 * 为前 num 个点光源槽位逐一查询 uniform 位置并缓存：
 *   - gPointLights[i].Color / Position / 三通道颜色与强度 / 衰减
 *   - gPointLightNum：当前激活的点光源数量
 *
 * 一次性缓存后，SetPointLight 每帧只做 glUniform* 调用，无字符串解析。
 * uniform 命名为结构体数组形式（gPointLights[i].Field），
 * 与 GLSL 中 "struct PointLight { ... } gPointLights[8];" 对应。
 */
void TechniqueLight::InitPointLightUniform(int num) {
    PointLightCountUniform = this->m_shader->GetUniformLocation("gPointLightNum");
    for (int i = 0; i < num; i++) {
        UniformPointLight uniform;
        string name;

        name = std::format("gPointLights[{}].Color", i);
        uniform.Color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].Position", i);
        uniform.Position = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AmbientIntensity", i);
        uniform.AmbientIntensity = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].DiffuseIntensity", i);
        uniform.DiffuseIntensity = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].DiffuseColor", i);
        uniform.DiffuseColor = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].SpecularColor", i);
        uniform.SpecularColor = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AmbienColor", i);
        uniform.AmbienColor = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationConstant", i);
        uniform.Atten.Constant = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationLinear", i);
        uniform.Atten.Linear = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationExp", i);
        uniform.Atten.Exp = this->m_shader->GetUniformLocation(name.c_str());

        PointLightUniforms.push_back(uniform);
    }
}

/*
 * 将单个点光源写入第 i 个 uniform 槽位
 *
 * 除写入各字段外，还会更新 gPointLightNum = i + 1，
 * 使着色器知道本次绘制实际参与的光源数量（用于循环上限，避免遍历未初始化槽位）。
 */
void TechniqueLight::SetPointLight(int i, PointLight *light) {
    this->m_shader->SetUniformValue(PointLightUniforms[i].Color, light->Color);
    this->m_shader->SetUniformValue(PointLightUniforms[i].Position, light->Position);
    this->m_shader->SetUniformValue(PointLightUniforms[i].AmbientIntensity, light->AmbientIntensity);
    this->m_shader->SetUniformValue(PointLightUniforms[i].DiffuseIntensity, light->DiffuseIntensity);
    this->m_shader->SetUniformValue(PointLightUniforms[i].DiffuseColor, light->DiffuseColor);
    this->m_shader->SetUniformValue(PointLightUniforms[i].SpecularColor, light->SpecularColor);
    this->m_shader->SetUniformValue(PointLightUniforms[i].AmbienColor, light->AmbientColor);
    this->m_shader->SetUniformValue(PointLightUniforms[i].Atten.Constant, light->Attenuation.Constant);
    this->m_shader->SetUniformValue(PointLightUniforms[i].Atten.Linear, light->Attenuation.Linear);
    this->m_shader->SetUniformValue(PointLightUniforms[i].Atten.Exp, light->Attenuation.Exp);

    this->m_shader->SetUniformValue(PointLightCountUniform, i + 1);
}

/*
 * 设置材质属性 uniform
 *
 * 将 CPU 侧 Material 的环境/漫反射/镜面反射颜色与光泽度同步到
 * 着色器 gMaterial 结构体（GLSL 材质系数）。
 */
void TechniqueLight::SetMaterial(const Material *m) {
    if (m == nullptr) {
        return;
    }
    MaterialUniform.SetAmbientColor(this->m_shader, m->AmbientColor);
    MaterialUniform.SetDiffuseColor(this->m_shader, m->DiffuseColor);
    MaterialUniform.SetSpecularColor(this->m_shader, m->SpecularColor);
    MaterialUniform.SetShininess(this->m_shader, m->Shininess);
}

void TechniqueLight::SetPointLights(vector<PointLight *> lights) {
    for (int i = 0; i < lights.size(); i++) {
        SetPointLight(i, lights[i]);
    }
}

MaterialUniform::MaterialUniform() {
}

MaterialUniform::~MaterialUniform() {
}

// ---- 材质 uniform 辅助类 ----
// 每种 setter 通过缓存好的 GLuint location 直接写入对应 gMaterial.* 字段

void MaterialUniform::SetAmbientColor(Shader *shader, const glm::vec3 &color) {
    shader->SetUniformValue(AmbientColor, color);
}

void MaterialUniform::SetDiffuseColor(Shader *shader, const glm::vec3 &color) {
    shader->SetUniformValue(DiffuseColor, color);
}

void MaterialUniform::SetSpecularColor(Shader *shader, const glm::vec3 &color) {
    shader->SetUniformValue(SpecularColor, color);
}

void MaterialUniform::SetShininess(Shader *shader, float shininess) {
    shader->SetUniformValue(Shininess, shininess);
}

/*
 * 初始化材质 uniform 位置
 *
 * 在着色器编译链接成功后调用，一次性查询 gMaterial 结构体各字段的 location。
 */
void MaterialUniform::Init(Shader *shader) {
    AmbientColor = shader->GetUniformLocation("gMaterial.AmbientColor");
    DiffuseColor = shader->GetUniformLocation("gMaterial.DiffuseColor");
    SpecularColor = shader->GetUniformLocation("gMaterial.SpecularColor");
    Shininess = shader->GetUniformLocation("gMaterial.Shininess");
}

// 预留：批量应用材质的方法，当前未使用
void MaterialUniform::Apply(Shader *shader) {
}