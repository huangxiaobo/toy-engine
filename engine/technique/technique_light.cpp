#include "technique_light.h"

#include "../light/direction_light.h"

TechniqueLight::TechniqueLight(QString name, QString vertexShader, QString fragmentShader)
    : Technique(name, vertexShader, fragmentShader)
{
}

TechniqueLight::~TechniqueLight()
{
}

void TechniqueLight::init()
{
    Technique::init();

    LightColorUniform = this->shader_program->uniformLocation("LightColorUniform");             // 获取顶点着色器中顶点属性 aPos 的位置
    LightPositionUniform = this->shader_program->uniformLocation("LightPositionUniform");       // 获取顶点着色器中顶点属性 aPos 的位置
    LightAmbientUniform = this->shader_program->uniformLocation("LightAmbientUniform");         // 获取顶点着色器中顶点属性 aPos 的位置
    LightAttenuationUniform = this->shader_program->uniformLocation("LightAttenuationUniform"); // 获取顶点着色器中顶点属性 aPos 的位置
    LightDiffuseUniform = this->shader_program->uniformLocation("LightDiffuseUniform");         // 获取顶点着色器中顶点属性 aPos 的位置
    LightSpecularUniform = this->shader_program->uniformLocation("LightSpecularUniform");
}

void TechniqueLight::SetDirectionLight(DirectionLight *light)
{
    this->shader_program->setUniformValue(LightColorUniform, light->getColor());
    this->shader_program->setUniformValue(LightAmbientUniform, light->getAmbient());
    this->shader_program->setUniformValue(LightDiffuseUniform, light->getDiffuse());
    this->shader_program->setUniformValue(LightSpecularUniform, light->getSpecular());
    this->shader_program->setUniformValue(LightDirectionUniform, light->getDirection());
}