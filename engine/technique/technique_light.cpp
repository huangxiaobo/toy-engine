#include "technique_light.h"

#include "../light/light.h"
#include "../shader/shader.h"
#include  "../material/material.h"

#include <iostream>
#include <format>

TechniqueLight::TechniqueLight(string name, string vertexShader, string fragmentShader)
    : Technique(name, vertexShader, fragmentShader)
{
    m_type = TechniqueTypeLight;
}

TechniqueLight::~TechniqueLight()
{
}

void TechniqueLight::init()
{
    Technique::init();
    InitPointLightUniform(8);
    MaterialUniform.Init(this->m_shader);
}

void TechniqueLight::SetDirectionLight(DirectionLight *light)
{
    this->m_shader->setUniformValue(LightColorUniform, light->Color);
    this->m_shader->setUniformValue(LightAmbientUniform, light->AmbientColor);
    this->m_shader->setUniformValue(LightDiffuseUniform, light->DiffuseColor);
    this->m_shader->setUniformValue(LightSpecularUniform, light->SpecularColor);
}

void TechniqueLight::InitPointLightUniform(int num)
{
    PointLightCountUniform = this->m_shader->uniformLocation("gPointLightNum");
    for (int i = 0; i < num; i++)
    {
        UniformPointLight uniform;
        string name;

        name = std::format("gPointLights[{}].Color", i);
        uniform.Color = this->m_shader->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].Position", i);
        uniform.Position = this->m_shader->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].DiffuseColor", i);
        uniform.DiffuseColor = this->m_shader->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].SpecularColor", i);
        uniform.SpecularColor = this->m_shader->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AmbienColor", i);
        uniform.AmbienColor = this->m_shader->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationConstant", i);
        uniform.Atten.Constant = this->m_shader->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationLinear", i);
        uniform.Atten.Linear = this->m_shader->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationExp", i);
        uniform.Atten.Exp = this->m_shader->uniformLocation(name.c_str());

        PointLightUniforms.push_back(uniform);
    }
}

void TechniqueLight::SetPointLight(int i, PointLight *light)
{
    this->m_shader->setUniformValue(PointLightUniforms[i].Color, light->Color);
    this->m_shader->setUniformValue(PointLightUniforms[i].Position, light->Position);
    this->m_shader->setUniformValue(PointLightUniforms[i].DiffuseColor, light->DiffuseColor);
    this->m_shader->setUniformValue(PointLightUniforms[i].SpecularColor, light->SpecularColor);
    this->m_shader->setUniformValue(PointLightUniforms[i].AmbienColor, light->AmbientColor);
    this->m_shader->setUniformValue(PointLightUniforms[i].Atten.Constant, light->Attenuation.Constant);
    this->m_shader->setUniformValue(PointLightUniforms[i].Atten.Linear, light->Attenuation.Linear);
    this->m_shader->setUniformValue(PointLightUniforms[i].Atten.Exp, light->Attenuation.Exp);

    this->m_shader->setUniformValue(PointLightCountUniform, i + 1);
}

void TechniqueLight::SetMaterial(Material *m)
{
    MaterialUniform.SetAmbientColor(this->m_shader, m->AmbientColor);
    MaterialUniform.SetDiffuseColor(this->m_shader, m->DiffuseColor);
    MaterialUniform.SetSpecularColor(this->m_shader, m->SpecularColor);
    MaterialUniform.SetShininess(this->m_shader, m->Shininess);
}

void TechniqueLight::SetPointLights(vector<PointLight *> lights)
{
    for (int i = 0; i < lights.size(); i++)
    {
        SetPointLight(i, lights[i]);
    }
}

MaterialUniform::MaterialUniform()
{
}

MaterialUniform::~MaterialUniform()
{
}

void MaterialUniform::SetAmbientColor(Shader* shader, const glm::vec3 &color)
{
    shader->setUniformValue(AmbientColor, color);
}

void MaterialUniform::SetDiffuseColor(Shader* shader, const glm::vec3 &color)
{
    shader->setUniformValue(DiffuseColor, color);
}

void MaterialUniform::SetSpecularColor(Shader* shader, const glm::vec3 &color)
{
    shader->setUniformValue(SpecularColor, color);
}

void MaterialUniform::SetShininess(Shader* shader, float shininess)
{
    shader->setUniformValue(Shininess, shininess);
}

void MaterialUniform::Init(Shader* shader)
{
    AmbientColor = shader->uniformLocation("gMaterial.AmbientColor");
    DiffuseColor = shader->uniformLocation("gMaterial.DiffuseColor");
    SpecularColor = shader->uniformLocation("gMaterial.SpecularColor");
    Shininess = shader->uniformLocation("gMaterial.Shininess");
}

void MaterialUniform::Apply(Shader* shader)
{
}
