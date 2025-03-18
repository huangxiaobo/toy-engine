#include "technique.h"
#include "../shader/shader.h"

#include <iostream>

Technique::Technique(string name, string vertex_shader, string fragment_shader) : m_type(TechniqueType::TechniqueTypeBase)
{
    this->m_shader = new Shader(
        vertex_shader.c_str(),
        fragment_shader.c_str());

    bool success = this->m_shader->Link();
    if (!success)
    {
        exit(-1);
    }

    this->m_shader->Use();                                                   // 如果使用 QShaderProgram，那么最好在获取顶点属性位置前，先 bind()
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

void Technique::SetWVPMatrix(const glm::mat4 &wvp)
{
    this->m_shader->SetUniformValue(m_uniform_wvp, wvp);
}

void Technique::SetCamera(const glm::vec3 &camera)
{
    this->m_shader->SetUniformValue(m_uniform_viewpos, camera);
}

void Technique::SetProjectionMatrix(const glm::mat4 &projection)
{
    this->m_shader->SetUniformValue(m_uniform_projection, projection);
}

void Technique::SetViewMatrix(const glm::mat4 &view)
{
    this->m_shader->SetUniformValue(m_uniform_view, view);
}

void Technique::SetModelMatrix(const glm::mat4 &model)
{
    this->m_shader->SetUniformValue(m_uniform_model, model);
}

void Technique::SetEyeWorldPos(const glm::vec3 &pos)
{
    this->m_shader->SetUniformValue(m_uniform_viewpos, pos);
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

void Technique::SetTextureUnit(unsigned int textureUnit)
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
    return m_shader->Use();
}

void Technique::Disable()
{
    m_shader->UnUse();
}

// 获取一个默认的Technique
Technique *Technique::GetDefaultTechnique()
{
    return new Technique("default", "./resource/shader/default.vert", "./resource/shader/default.frag");
}