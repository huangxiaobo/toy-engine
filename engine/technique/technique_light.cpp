#include "technique_light.h"

#include "../light/light.h"
#include "../shader/shader.h"
#include "../material/material.h"
#include "../utils/utils.h"

#include <iostream>
#include <format>

TechniqueLight::TechniqueLight(string name, string vertexShader, string fragmentShader)
    : Technique(name, vertexShader, fragmentShader)
{
    m_type = TechniqueTypeLight;

    InitPointLightUniform(8);
    MaterialUniform.Init(this->m_shader);
}

TechniqueLight::~TechniqueLight()
{
}

void TechniqueLight::init()
{
}

void TechniqueLight::SetLights(const vector<Light *> &lights)
{
    int point_light_count = 0;
    for (auto light : lights)
    {
        switch (light->GetLightType())
        {
        case LightTypeDirection:
            SetDirectionLight((DirectionLight *)light);
            break;
        case LightTypePoint:
            SetPointLight(point_light_count++, (PointLight *)light);
            break;
        case LightTypeSpot:
            break;
        default:
            break;
        }
    }
}

void TechniqueLight::SetDirectionLight(DirectionLight *light)
{
    this->m_shader->SetUniformValue(LightColorUniform, light->Color);
    this->m_shader->SetUniformValue(LightAmbientUniform, light->AmbientColor);
    this->m_shader->SetUniformValue(LightDiffuseUniform, light->DiffuseColor);
    this->m_shader->SetUniformValue(LightSpecularUniform, light->SpecularColor);
}

void TechniqueLight::InitPointLightUniform(int num)
{
    PointLightCountUniform = this->m_shader->GetUniformLocation("gPointLightNum");
    for (int i = 0; i < num; i++)
    {
        UniformPointLight uniform;
        string name;

        name = std::format("gPointLights[{}].Color", i);
        uniform.Color = this->m_shader->GetUniformLocation(name.c_str());

        name = std::format("gPointLights[{}].Position", i);
        uniform.Position = this->m_shader->GetUniformLocation(name.c_str());

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

void TechniqueLight::SetPointLight(int i, PointLight *light)
{
    this->m_shader->SetUniformValue(PointLightUniforms[i].Color, light->Color);
    this->m_shader->SetUniformValue(PointLightUniforms[i].Position, light->Position);
    this->m_shader->SetUniformValue(PointLightUniforms[i].DiffuseColor, light->DiffuseColor);
    this->m_shader->SetUniformValue(PointLightUniforms[i].SpecularColor, light->SpecularColor);
    this->m_shader->SetUniformValue(PointLightUniforms[i].AmbienColor, light->AmbientColor);
    this->m_shader->SetUniformValue(PointLightUniforms[i].Atten.Constant, light->Attenuation.Constant);
    this->m_shader->SetUniformValue(PointLightUniforms[i].Atten.Linear, light->Attenuation.Linear);
    this->m_shader->SetUniformValue(PointLightUniforms[i].Atten.Exp, light->Attenuation.Exp);

    this->m_shader->SetUniformValue(PointLightCountUniform, i + 1);
}

void TechniqueLight::SetMaterial(const Material *m)
{
    if (m == nullptr)
    {
        return;
    }
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

void MaterialUniform::SetAmbientColor(Shader *shader, const glm::vec3 &color)
{
    shader->SetUniformValue(AmbientColor, color);
}

void MaterialUniform::SetDiffuseColor(Shader *shader, const glm::vec3 &color)
{
    shader->SetUniformValue(DiffuseColor, color);
}

void MaterialUniform::SetSpecularColor(Shader *shader, const glm::vec3 &color)
{
    shader->SetUniformValue(SpecularColor, color);
}

void MaterialUniform::SetShininess(Shader *shader, float shininess)
{
    shader->SetUniformValue(Shininess, shininess);
}

void MaterialUniform::Init(Shader *shader)
{
    AmbientColor = shader->GetUniformLocation("gMaterial.AmbientColor");
    DiffuseColor = shader->GetUniformLocation("gMaterial.DiffuseColor");
    SpecularColor = shader->GetUniformLocation("gMaterial.SpecularColor");
    Shininess = shader->GetUniformLocation("gMaterial.Shininess");
}

void MaterialUniform::Apply(Shader *shader)
{
}
