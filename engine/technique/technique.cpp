#include "technique.h"
#include "../shader/shader.h"

#include <iostream>

Technique::Technique(string name,  string vertex_shader,  string fragment_shader):m_type(TechniqueType::TechniqueTypeBase)
{
    m_shader_vert = vertex_shader;
    m_shader_frag = fragment_shader;
}

Technique::~Technique()
{
    if (m_shader != nullptr)
    {
        delete m_shader;
        m_shader = nullptr;
    }
}

void Technique::init()
{
    this->m_shader = new Shader();
    this->m_shader->init();
    // ===================== 着色器 =====================
    this->m_shader->addShaderFromSourceFile(ShaderType::VERTEX_SHADER, m_shader_vert.c_str());
    this->m_shader->addShaderFromSourceFile(ShaderType::FRAGMENT_SHADER, m_shader_frag.c_str());

    bool success = this->m_shader->link();
    if (!success){
        exit(-1);
    }

    this->m_shader->bind();                                            // 如果使用 QShaderProgram，那么最好在获取顶点属性位置前，先 bind()
    ProjectionUniform = this->m_shader->uniformLocation("projection"); // 获取顶点着色器中顶点属性 aPos 的位置
    ViewUniform = this->m_shader->uniformLocation("view");             // 获取顶点着色器中顶点属性 aPos 的位置
    ModelUniform = this->m_shader->uniformLocation("model");           // 获取顶点着色器中顶点属性 aPos 的位置
    WvpUniform = this->m_shader->uniformLocation("gWVP");              // 获取顶点着色器中顶点属性 aPos 的位置
    CameraUniform = this->m_shader->uniformLocation("gViewPos");       // 获取顶点着色器中顶点属性 aPos 的位置
}

void Technique::SetWVP(const glm::mat4 &wvp)
{
    this->m_shader->setUniformValue(WvpUniform, wvp);
}

void Technique::SetCamera(const glm::vec3& camera)
{
    this->m_shader->setUniformValue(CameraUniform, camera);
}

void Technique::SetProjection(const glm::mat4& projection)
{
    this->m_shader->setUniformValue(ProjectionUniform, projection);
}

void Technique::SetView(const glm::mat4& view)
{
    this->m_shader->setUniformValue(ViewUniform, view);
}

void Technique::SetModel( const glm::mat4& model)
{
    this->m_shader->setUniformValue(ModelUniform, model);
}

void Technique::setUniform(const char* name, const glm::vec2& value)
{
}

void Technique::setUniform(const char* name, const glm::vec3& value)
{
}

void Technique::setUniform(const char* name, const glm::vec4& value)
{
}

void Technique::setUniform(const char* name, float value)
{
}

void Technique::setUniform(const char* name, int value)
{
}

void Technique::setUniform()
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