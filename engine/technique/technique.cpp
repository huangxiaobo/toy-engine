#include "technique.h"
#include "../shader/shader.h"

#include <iostream>

Technique::Technique(string name, string vertex_shader, string fragment_shader) : m_type(TechniqueType::TechniqueTypeBase)
{
    this->m_shader = new Shader();
    this->m_shader->init();
    // ===================== 着色器 =====================
    this->m_shader->addShaderFromSourceFile(ShaderType::VERTEX_SHADER, vertex_shader.c_str());
    this->m_shader->addShaderFromSourceFile(ShaderType::FRAGMENT_SHADER, fragment_shader.c_str());

    bool success = this->m_shader->link();
    if (!success)
    {
        exit(-1);
    }

    this->m_shader->bind();                                                  // 如果使用 QShaderProgram，那么最好在获取顶点属性位置前，先 bind()
    m_uniform_projection = this->m_shader->GetUniformLocation("projection"); // 获取顶点着色器中顶点属性 aPos 的位置
    m_uniform_view = this->m_shader->GetUniformLocation("view");             // 获取顶点着色器中顶点属性 aPos 的位置
    m_uniform_model = this->m_shader->GetUniformLocation("model");           // 获取顶点着色器中顶点属性 aPos 的位置
    m_uniform_wvp = this->m_shader->GetUniformLocation("gWVP");              // 获取顶点着色器中顶点属性 aPos 的位置
    m_uniform_viewpos = this->m_shader->GetUniformLocation("gViewPos");      // 获取顶点着色器中顶点属性 aPos 的位置
}

Technique::~Technique()
{
    if (m_shader != nullptr)
    {
        delete m_shader;
        m_shader = nullptr;
    }
}

Shader *Technique::GetShader() const
{
    return m_shader;
}

void Technique::SetWVP(const glm::mat4 &wvp)
{
    this->m_shader->SetUniformValue(m_uniform_wvp, wvp);
}

void Technique::SetCamera(const glm::vec3 &camera)
{
    this->m_shader->SetUniformValue(m_uniform_viewpos, camera);
}

void Technique::SetProjection(const glm::mat4 &projection)
{
    this->m_shader->SetUniformValue(m_uniform_projection, projection);
}

void Technique::SetView(const glm::mat4 &view)
{
    this->m_shader->SetUniformValue(m_uniform_view, view);
}

void Technique::SetModel(const glm::mat4 &model)
{
    this->m_shader->SetUniformValue(m_uniform_model, model);
}

void Technique::SetUniform(const char *name, const glm::vec2 &value)
{
}

void Technique::SetUniform(const char *name, const glm::vec3 &value)
{
}

void Technique::SetUniform(const char *name, const glm::vec4 &value)
{
}

void Technique::SetUniform(const char *name, float value)
{
}

void Technique::SetUniform(const char *name, int value)
{
}

void Technique::SetUniform()
{
}

void Technique::SetLights(const vector<Light *> &lights)
{
}

void Technique::SetMaterial(const Material *material)
{
}

void Technique::Enable()
{
    bool ok = m_shader->bind();
}

void Technique::Disable()
{
    m_shader->unbind();
}