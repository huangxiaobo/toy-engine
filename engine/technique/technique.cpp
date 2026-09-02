#include "technique.h"
#include "../shader/shader.h"
#include "../utils/utils.h"

#include <iostream>

/*
 * Technique 基类：封装"着色器 + 常用矩阵/相机 uniform"的渲染技术
 *
 * 职责：
 *   1. 创建并链接 Shader（编译失败/链接失败时 exit 终止）
 *   2. 预取投影/视图/模型/WVP/相机位置等常用 uniform 的 location，避免每帧字符串查询
 *   3. 提供矩阵/向量/标量的 uniform 设置接口
 *
 * 子类（TechniqueLight）在此基础上扩展灯光与材质支持。
 */
Technique::Technique(string name, string vertex_shader, string fragment_shader) : m_type(
    TechniqueType::TechniqueTypeBase) {
    // 构造时自动生成唯一 UUID
    Id = Utils::GenerateUUID();

    this->m_shader = new Shader(
        vertex_shader.c_str(),
        fragment_shader.c_str());

    bool success = this->m_shader->Link();
    if (!success) {
        exit(-1);
    }

    this->m_shader->Use(); // 如果使用 QShaderProgram，那么最好在获取顶点属性位置前，先 bind()
    m_uniform_projection = this->m_shader->GetUniformLocation("projection"); // 获取顶点着色器中顶点属性 projection 的位置
    m_uniform_view = this->m_shader->GetUniformLocation("view"); // 获取顶点着色器中顶点属性 view 的位置
    m_uniform_model = this->m_shader->GetUniformLocation("model"); // 获取顶点着色器中顶点属性 model 的位置
    m_uniform_wvp = this->m_shader->GetUniformLocation("gWVP"); // 获取顶点着色器中顶点属性 gWVP 的位置
    m_uniform_viewpos = this->m_shader->GetUniformLocation("gViewPos"); // 获取顶点着色器中顶点属性 gViewPos 的位置
}

Technique::~Technique() {
    if (m_shader != nullptr) {
        delete m_shader;
        m_shader = nullptr;
    }
}

Shader *Technique::GetShader() const {
    return m_shader;
}

void Technique::SetWVPMatrix(const glm::mat4 &wvp) {
    this->m_shader->SetUniformValue(m_uniform_wvp, wvp);
}

// 设置观察空间相机位置（gViewPos），供着色器做视差/高光等基于视角的计算
void Technique::SetCamera(const glm::vec3 &camera) {
    this->m_shader->SetUniformValue(m_uniform_viewpos, camera);
}

void Technique::SetProjectionMatrix(const glm::mat4 &projection) {
    this->m_shader->SetUniformValue(m_uniform_projection, projection);
}

void Technique::SetViewMatrix(const glm::mat4 &view) {
    this->m_shader->SetUniformValue(m_uniform_view, view);
}

void Technique::SetModelMatrix(const glm::mat4 &model) {
    this->m_shader->SetUniformValue(m_uniform_model, model);
}

// SetCamera 的别名：设置相机（眼睛）在世界空间的位置
void Technique::SetEyeWorldPos(const glm::vec3 &pos) {
    this->m_shader->SetUniformValue(m_uniform_viewpos, pos);
}

// ---- 按名称设置任意 uniform 的重载族（供子类/调用方传入自定义 uniform 名） ----

void Technique::SetUniform(const char *name, const glm::vec2 &value) {
    this->m_shader->SetUniformValue(name, value);
}

void Technique::SetUniform(const char *name, const glm::vec3 &value) {
    this->m_shader->SetUniformValue(name, value);
}

void Technique::SetUniform(const char *name, const glm::vec4 &value) {
    this->m_shader->SetUniformValue(name, value);
}

void Technique::SetUniform(const char *name, float value) {
    this->m_shader->SetUniformValue(name, value);
}

void Technique::SetUniform(const char *name, int value) {
    this->m_shader->SetUniformValue(name, value);
}

// 空实现预留
void Technique::SetUniform() {
}

// 空实现预留：设置纹理单元绑定
void Technique::SetTextureUnit(unsigned int textureUnit) {
}

// 基类不支持灯光：空实现，由 TechniqueLight 覆写
void Technique::SetLights(const vector<Light *> &lights) {
}

// 基类不支持材质：空实现，由 TechniqueLight 覆写
void Technique::SetMaterial(const Material *material) {
}

/*
 * 激活该技术用于绘制
 *
 * 启用着色器 program，并绑定片元输出到 location 0（保证多渲染目标时输出正确）。
 * 调用时机：每帧绘制某个对象前调用，随后通过 Set*Matrix 设置变换。
 */
void Technique::Enable() {
    m_shader->Use();
    m_shader->BindFragDataLocation();
}

// 停用当前 program（解绑）
void Technique::Disable() {
    m_shader->UnUse();
}

// 获取一个默认的Technique
Technique *Technique::GetDefaultTechnique() {
    return new Technique("default", "./resource/shader/default.vert", "./resource/shader/default.frag");
}