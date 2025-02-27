#include "technique_light.h"

#include "../light/light.h"
#include "../shader/shader.h"
#include  "../material/material.h"

#include <iostream>
#include <format>

TechniqueLight::TechniqueLight(string name, string vertexShader, string fragmentShader)
    : Technique(name, vertexShader, fragmentShader)
{
    type = TechniqueTypeLight;
}

TechniqueLight::~TechniqueLight()
{
}

void TechniqueLight::init()
{
    Technique::init();
    InitPointLightUniform(8);
    MaterialUniform.Init(this->shader_program);
}

void TechniqueLight::SetDirectionLight(DirectionLight *light)
{
    this->shader_program->setUniformValue(LightColorUniform, light->Color);
    this->shader_program->setUniformValue(LightAmbientUniform, light->AmbientColor);
    this->shader_program->setUniformValue(LightDiffuseUniform, light->DiffuseColor);
    this->shader_program->setUniformValue(LightSpecularUniform, light->SpecularColor);
}

void TechniqueLight::InitPointLightUniform(int num)
{
    PointLightCountUniform = this->shader_program->uniformLocation("gPointLightNum");
    for (int i = 0; i < num; i++)
    {
        UniformPointLight uniform;
        string name;

        name = std::format("gPointLights[{}].Color", i);
        uniform.Color = this->shader_program->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].Position", i);
        uniform.Position = this->shader_program->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].DiffuseColor", i);
        uniform.DiffuseColor = this->shader_program->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].SpecularColor", i);
        uniform.SpecularColor = this->shader_program->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AmbienColor", i);
        uniform.AmbienColor = this->shader_program->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationConstant", i);
        uniform.Atten.Constant = this->shader_program->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationLinear", i);
        uniform.Atten.Linear = this->shader_program->uniformLocation(name.c_str());

        name = std::format("gPointLights[{}].AttenuationExp", i);
        uniform.Atten.Exp = this->shader_program->uniformLocation(name.c_str());

        PointLightUniforms.push_back(uniform);
    }
}

void TechniqueLight::SetPointLight(int i, PointLight *light)
{
    this->shader_program->setUniformValue(PointLightUniforms[i].Color, light->Color);
    this->shader_program->setUniformValue(PointLightUniforms[i].Position, light->Position);
    this->shader_program->setUniformValue(PointLightUniforms[i].DiffuseColor, light->DiffuseColor);
    this->shader_program->setUniformValue(PointLightUniforms[i].SpecularColor, light->SpecularColor);
    this->shader_program->setUniformValue(PointLightUniforms[i].AmbienColor, light->AmbientColor);
    this->shader_program->setUniformValue(PointLightUniforms[i].Atten.Constant, light->Attenuation.Constant);
    this->shader_program->setUniformValue(PointLightUniforms[i].Atten.Linear, light->Attenuation.Linear);
    this->shader_program->setUniformValue(PointLightUniforms[i].Atten.Exp, light->Attenuation.Exp);

    this->shader_program->setUniformValue(PointLightCountUniform, i + 1);
}

void TechniqueLight::SetMaterial(Material *m)
{
    MaterialUniform.SetAmbientColor(this->shader_program, m->AmbientColor);
    MaterialUniform.SetDiffuseColor(this->shader_program, m->DiffuseColor);
    MaterialUniform.SetSpecularColor(this->shader_program, m->SpecularColor);
    MaterialUniform.SetShininess(this->shader_program, m->Shininess);
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
