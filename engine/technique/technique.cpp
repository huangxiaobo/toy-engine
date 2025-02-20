#include "technique.h"
#include "../shader/shader.h"

#include <iostream>

Technique::Technique(string name,  string vertexShader,  string fragmentShader)
{
    shaderVertex = vertexShader;
    shaderFragment = fragmentShader;
}

Technique::~Technique()
{
    if (shader_program != nullptr)
    {
        delete shader_program;
        shader_program = nullptr;
    }
}

void Technique::init()
{
    this->shader_program = new Shader();
    this->shader_program->init();
    // ===================== 着色器 =====================
    this->shader_program->addShaderFromSourceFile(ShaderType::VERTEX_SHADER, shaderVertex.c_str());
    this->shader_program->addShaderFromSourceFile(ShaderType::FRAGMENT_SHADER, shaderFragment.c_str());

    bool success = this->shader_program->link();
    if (!success){
        exit(-1);
    }

    this->shader_program->bind();                                            // 如果使用 QShaderProgram，那么最好在获取顶点属性位置前，先 bind()
    ProjectionUniform = this->shader_program->uniformLocation("projection"); // 获取顶点着色器中顶点属性 aPos 的位置
    ViewUniform = this->shader_program->uniformLocation("view");             // 获取顶点着色器中顶点属性 aPos 的位置
    ModelUniform = this->shader_program->uniformLocation("model");           // 获取顶点着色器中顶点属性 aPos 的位置
    WvpUniform = this->shader_program->uniformLocation("gWVP");              // 获取顶点着色器中顶点属性 aPos 的位置
    CameraUniform = this->shader_program->uniformLocation("gViewPos");       // 获取顶点着色器中顶点属性 aPos 的位置

}

void Technique::SetWVP(const glm::mat4& wvp)
{
    this->shader_program->setUniformValue(WvpUniform, wvp);
}

void Technique::SetCamera(const glm::vec3& camera)
{
    this->shader_program->setUniformValue(CameraUniform, camera);
}

void Technique::SetProjection(const glm::mat4& projection)
{
    this->shader_program->setUniformValue(ProjectionUniform, projection);
}

void Technique::SetView(const glm::mat4& view)
{
    this->shader_program->setUniformValue(ViewUniform, view);
}

void Technique::SetModel( const glm::mat4& model)
{
    this->shader_program->setUniformValue(ModelUniform, model);
}

void Technique::draw(long long elapsed)
{
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
    bool ok = shader_program->bind();
}

void Technique::Disable()
{
    shader_program->unbind();
}