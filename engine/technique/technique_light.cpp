#include "technique_light.h"

#include "../light/light.h"
#include "../shader/shader.h"
#include "../material/material.h"
#include "../utils/utils.h"

#include <iostream>
#include <format>

using namespace std;

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

    InitDirectionLightUniform();
    InitPointLightUniform(8);
    InitSpotLightUniform(8);
    // m_shader 为基类持有的 unique_ptr，此处传入裸指针供 uniform 缓存
    m_material_uniform.Init(this->m_shader.get());
}

TechniqueLight::~TechniqueLight() {
}

/*
 * 批量设置场景中的全部灯光
 *
 * 遍历光线列表，按类型分发：
 *   - 方向光 -> SetDirectionLight（填充方向光 uniform）
 *   - 点光源 -> SetPointLight（写入点光源数组 uniform，并自动递增计数器）
 *   - 聚光灯 -> SetSpotLight（写入聚光灯数组 uniform，并自动递增计数器）
 *
 * 注意：此方法在每帧绘制前由 Renderer 调用（如地形/模型 Draw 前 SetLights），
 * 将 CPU 侧灯光数据同步到 GPU uniform。
 */
void TechniqueLight::SetLights(const vector<Light *> &lights) {
    int point_light_count = 0;
    int spot_light_count = 0;
    for (auto light: lights) {
        // 跳过已禁用的灯光：点光源/聚光灯不写入且不计数；
        // 方向光需单独置零清除其贡献（见 SetDirectionLight）
        if (!light->IsEnabled()) {
            if (light->GetLightType() == LightTypeDirection) {
                SetDirectionLight(nullptr);
            }
            continue;
        }
        switch (light->GetLightType()) {
            case LightTypeDirection:
                SetDirectionLight((DirectionLight *) light);
                break;
            case LightTypePoint:
                SetPointLight(point_light_count++, (PointLight *) light);
                break;
            case LightTypeSpot:
                SetSpotLight(spot_light_count++, (SpotLight *) light);
                break;
            default:
                break;
        }
    }

    // 汇总时显式写回最终的光源数量 uniform，确保对象被禁用（尤其禁用最后一个/全部）时，
    // gPointLightNum / gSpotLightNum 能如实反映当前激活数量，而不是残留上一帧更高的值，
    // 从而避免着色器仍按旧数量遍历到已禁用光源遗留的脏数据（禁用不生效的根因）。
    this->m_shader->SetUniformValue(m_point_light_count_uniform, point_light_count);
    this->m_shader->SetUniformValue(m_spot_light_count_uniform, spot_light_count);
}

/*
 * 设置方向光（平行光）的 uniform
 *
 * 方向光有方向、颜色/环境/漫反射/镜面反射分量，无位置与衰减，
 * 着色器中表现为单个全局 uniform（非数组）。使用缓存的 uniform location
 * 写入 gDirectionLight 结构体各字段。
 *
 * 传入 nullptr 表示该方向光被禁用：此时将所有字段清零，
 * 保证着色器中累加的 gDirectionLight 贡献为 0（避免遗留上一帧的亮度）。
 */
void TechniqueLight::SetDirectionLight(DirectionLight *light) {
    if (light == nullptr) {
        // 禁用状态：清零方向光各字段，使其在片元着色器中不产生光照
        this->m_shader->SetUniformValue(m_direction_light_uniform.m_direction, glm::vec3(0.0f, -1.0f, 0.0f));
        this->m_shader->SetUniformValue(m_direction_light_uniform.m_color, glm::vec3(0.0f));
        this->m_shader->SetUniformValue(m_direction_light_uniform.m_ambient_intensity, 0.0f);
        this->m_shader->SetUniformValue(m_direction_light_uniform.m_diffuse_intensity, 0.0f);
        this->m_shader->SetUniformValue(m_direction_light_uniform.m_specular_intensity, 0.0f);
        this->m_shader->SetUniformValue(m_direction_light_uniform.m_ambient_color, glm::vec3(0.0f));
        this->m_shader->SetUniformValue(m_direction_light_uniform.m_diffuse_color, glm::vec3(0.0f));
        this->m_shader->SetUniformValue(m_direction_light_uniform.m_specular_color, glm::vec3(0.0f));
        return;
    }
    this->m_shader->SetUniformValue(m_direction_light_uniform.m_direction, light->m_direction);
    this->m_shader->SetUniformValue(m_direction_light_uniform.m_color, light->m_color);
    this->m_shader->SetUniformValue(m_direction_light_uniform.m_ambient_intensity, light->m_ambient_intensity);
    this->m_shader->SetUniformValue(m_direction_light_uniform.m_diffuse_intensity, light->m_diffuse_intensity);
    this->m_shader->SetUniformValue(m_direction_light_uniform.m_specular_intensity, light->m_specular_intensity);
    this->m_shader->SetUniformValue(m_direction_light_uniform.m_ambient_color, light->m_ambient_color);
    this->m_shader->SetUniformValue(m_direction_light_uniform.m_diffuse_color, light->m_diffuse_color);
    this->m_shader->SetUniformValue(m_direction_light_uniform.m_specular_color, light->m_specular_color);
}

/*
 * 预取点光源数组 uniform 位置
 *
 * 为前 num 个点光源槽位逐一查询 uniform 位置并缓存：
 *   - gPointLights[i].m_color / Position / 三通道颜色与强度 / 衰减
 *   - gPointLightNum：当前激活的点光源数量
 *
 * 一次性缓存后，SetPointLight 每帧只做 glUniform* 调用，无字符串解析。
 * uniform 命名为结构体数组形式（gPointLights[i].Field），
 * 与 GLSL 中 "struct PointLight { ... } gPointLights[8];" 对应。
 */
void TechniqueLight::InitPointLightUniform(int num) {
    m_point_light_count_uniform = this->m_shader->GetUniformLocation("gPointLightNum");
    for (int i = 0; i < num; i++) {
        UniformPointLight uniform;
        string name;

        name = std::format("gPointLights[{}].Color", i);
        uniform.m_color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].Position", i);
        uniform.m_position = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AmbientIntensity", i);
        uniform.m_ambient_intensity = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].DiffuseIntensity", i);
        uniform.m_diffuse_intensity = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].DiffuseColor", i);
        uniform.m_diffuse_color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].SpecularColor", i);
        uniform.m_specular_color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AmbientColor", i);
        uniform.m_ambient_color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationConstant", i);
        uniform.m_atten.m_constant = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationLinear", i);
        uniform.m_atten.m_linear = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationExp", i);
        uniform.m_atten.m_exp = this->m_shader->GetUniformLocation(name.c_str());

        m_point_light_uniforms.push_back(uniform);
    }
}

/*
 * 预取方向光 uniform 位置
 *
 * 方向光在着色器中对应用户自定义结构体 UniformDirectionLight，字段命名如下，
 * 一次性缓存 location，避免每帧字符串查找：
 *   - gDirectionLight.m_direction / Color / 三通道颜色与强度
 */
void TechniqueLight::InitDirectionLightUniform() {
    m_direction_light_uniform.m_direction = this->m_shader->GetUniformLocation("gDirectionLight.Direction");
    m_direction_light_uniform.m_color = this->m_shader->GetUniformLocation("gDirectionLight.Color");
    m_direction_light_uniform.m_ambient_intensity = this->m_shader->GetUniformLocation("gDirectionLight.AmbientIntensity");
    m_direction_light_uniform.m_diffuse_intensity = this->m_shader->GetUniformLocation("gDirectionLight.DiffuseIntensity");
    m_direction_light_uniform.m_specular_intensity = this->m_shader->GetUniformLocation("gDirectionLight.SpecularIntensity");
    m_direction_light_uniform.m_ambient_color = this->m_shader->GetUniformLocation("gDirectionLight.AmbientColor");
    m_direction_light_uniform.m_diffuse_color = this->m_shader->GetUniformLocation("gDirectionLight.DiffuseColor");
    m_direction_light_uniform.m_specular_color = this->m_shader->GetUniformLocation("gDirectionLight.SpecularColor");
}

/*
 * 预取聚光灯数组 uniform 位置
 *
 * 为前 num 个聚光灯槽位逐一查询 uniform 位置并缓存（含锥角 Cutoff/OuterCutoff），
 * 与 GLSL 中 "struct SpotLight { ... } gSpotLights[8];" 对应。
 */
void TechniqueLight::InitSpotLightUniform(int num) {
    m_spot_light_count_uniform = this->m_shader->GetUniformLocation("gSpotLightNum");
    for (int i = 0; i < num; i++) {
        UniformSpotLight uniform;
        string name;

        name = std::format("gSpotLights[{}].Position", i);
        uniform.m_position = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].Direction", i);
        uniform.m_direction = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].Color", i);
        uniform.m_color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].AmbientIntensity", i);
        uniform.m_ambient_intensity = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].DiffuseIntensity", i);
        uniform.m_diffuse_intensity = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].SpecularIntensity", i);
        uniform.m_specular_intensity = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].AmbientColor", i);
        uniform.m_ambient_color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].DiffuseColor", i);
        uniform.m_diffuse_color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].SpecularColor", i);
        uniform.m_specular_color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].AttenuationConstant", i);
        uniform.m_atten.m_constant = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].AttenuationLinear", i);
        uniform.m_atten.m_linear = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].AttenuationExp", i);
        uniform.m_atten.m_exp = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].Cutoff", i);
        uniform.m_cutoff = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gSpotLights[{}].OuterCutoff", i);
        uniform.m_outer_cutoff = this->m_shader->GetUniformLocation(name.c_str());

        m_spot_light_uniforms.push_back(uniform);
    }
}

/*
 * 将单个聚光灯写入第 i 个 uniform 槽位
 *
 * 除写入各字段外，还会更新 gSpotLightNum = i + 1，
 * 使着色器知道本次绘制实际参与的光源数量（用于循环上限，避免遍历未初始化槽位）。
 */
void TechniqueLight::SetSpotLight(int i, SpotLight *light) {
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_position, light->m_position);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_direction, light->m_direction);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_color, light->m_color);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_ambient_intensity, light->m_ambient_intensity);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_diffuse_intensity, light->m_diffuse_intensity);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_specular_intensity, light->m_specular_intensity);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_ambient_color, light->m_ambient_color);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_diffuse_color, light->m_diffuse_color);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_specular_color, light->m_specular_color);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_atten.m_constant, light->Attenuation.m_constant);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_atten.m_linear, light->Attenuation.m_linear);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_atten.m_exp, light->Attenuation.m_exp);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_cutoff, light->m_cutoff);
    this->m_shader->SetUniformValue(m_spot_light_uniforms[i].m_outer_cutoff, light->m_outer_cutoff);

    this->m_shader->SetUniformValue(m_spot_light_count_uniform, i + 1);
}

/*
 * 将单个点光源写入第 i 个 uniform 槽位
 *
 * 除写入各字段外，还会更新 gPointLightNum = i + 1，
 * 使着色器知道本次绘制实际参与的光源数量（用于循环上限，避免遍历未初始化槽位）。
 */
void TechniqueLight::SetPointLight(int i, PointLight *light) {
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_color, light->m_color);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_position, light->m_position);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_ambient_intensity, light->m_ambient_intensity);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_diffuse_intensity, light->m_diffuse_intensity);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_diffuse_color, light->m_diffuse_color);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_specular_color, light->m_specular_color);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_ambient_color, light->m_ambient_color);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_atten.m_constant, light->Attenuation.m_constant);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_atten.m_linear, light->Attenuation.m_linear);
    this->m_shader->SetUniformValue(m_point_light_uniforms[i].m_atten.m_exp, light->Attenuation.m_exp);

    this->m_shader->SetUniformValue(m_point_light_count_uniform, i + 1);
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
    // 必须先激活本 technique 的 shader program：init 阶段调用时没有 program 处于绑定状态，
    // 不 Use 的话 glUniform* 会全部落空
    this->m_shader->Use();
    m_material_uniform.SetAmbientColor(this->m_shader.get(), m->m_ambient_color);
    m_material_uniform.SetDiffuseColor(this->m_shader.get(), m->m_diffuse_color);
    m_material_uniform.SetSpecularColor(this->m_shader.get(), m->m_specular_color);
    m_material_uniform.SetShininess(this->m_shader.get(), m->m_shininess);
}

void TechniqueLight::SetPointLights(vector<PointLight *> lights) {
    for (size_t i = 0; i < lights.size(); i++) {
        SetPointLight(i, lights[i]);
    }
}

void TechniqueLight::SetSpotLights(vector<SpotLight *> lights) {
    for (size_t i = 0; i < lights.size(); i++) {
        SetSpotLight(i, lights[i]);
    }
}

MaterialUniform::MaterialUniform() {
}

MaterialUniform::~MaterialUniform() {
}

// ---- 材质 uniform 辅助类 ----
// 每种 setter 通过缓存好的 GLuint location 直接写入对应 gMaterial.* 字段

void MaterialUniform::SetAmbientColor(Shader *shader, const glm::vec3 &color) {
    shader->SetUniformValue(m_ambient_color, color);
}

void MaterialUniform::SetDiffuseColor(Shader *shader, const glm::vec3 &color) {
    shader->SetUniformValue(m_diffuse_color, color);
}

void MaterialUniform::SetSpecularColor(Shader *shader, const glm::vec3 &color) {
    shader->SetUniformValue(m_specular_color, color);
}

void MaterialUniform::SetShininess(Shader *shader, float shininess) {
    shader->SetUniformValue(m_shininess, shininess);
}

/*
 * 初始化材质 uniform 位置
 *
 * 在着色器编译链接成功后调用，一次性查询 gMaterial 结构体各字段的 location。
 */
void MaterialUniform::Init(Shader *shader) {
    m_ambient_color = shader->GetUniformLocation("gMaterial.AmbientColor");
    m_diffuse_color = shader->GetUniformLocation("gMaterial.DiffuseColor");
    m_specular_color = shader->GetUniformLocation("gMaterial.SpecularColor");
    m_shininess = shader->GetUniformLocation("gMaterial.Shininess");
}

// 预留：批量应用材质的方法，当前未使用
void MaterialUniform::Apply(Shader *shader) {
}