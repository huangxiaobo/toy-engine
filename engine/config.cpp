#include "config.h"

#include <tinyxml2/tinyxml2.h>

#include <iostream>
#include <vector>
Config::Config()
{
}

Config::~Config()
{
}

glm::vec3 GetRGB(tinyxml2::XMLElement *element)
{

    if (element != nullptr)
    {
        auto r = element->FirstChildElement("r")->FloatText();
        auto g = element->FirstChildElement("g")->FloatText();
        auto b = element->FirstChildElement("b")->FloatText();
        return glm::vec3(r, g, b);
    }
    return glm::vec3(0.0f);
}

glm::vec3 GetXYZ(tinyxml2::XMLElement *element)
{
    if (element != nullptr)
    {
        auto x = element->FirstChildElement("x")->FloatText();
        auto y = element->FirstChildElement("y")->FloatText();
        auto z = element->FirstChildElement("z")->FloatText();
        return glm::vec3(x, y, z);
    }
    return glm::vec3(0.0f);
}

Config *Config::LoadFromXml(std::string xml_file)
{

    Config *config = new Config();

    tinyxml2::XMLDocument doc;
    auto err = doc.LoadFile(xml_file.c_str());
    if (err != tinyxml2::XML_SUCCESS)
    {
        std::cout << "Load failed" << std::endl;
        exit(-1);
    }
    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    std::cout << printer.CStr() << std::endl;

    auto xml_window = doc.FirstChildElement("world")->FirstChildElement("window");
    config->Window.WindowWidth = xml_window->FirstChildElement("width")->IntText();
    config->Window.WindowHeight = xml_window->FirstChildElement("height")->IntText();

    auto xml_clip = doc.FirstChildElement("world")->FirstChildElement("clip");
    config->Clip.ClipNear = xml_clip->FirstChildElement("near")->FloatText();
    config->Clip.ClipFar = xml_clip->FirstChildElement("far")->FloatText();
    config->Clip.ClipFov = xml_clip->FirstChildElement("fov")->FloatText();
    config->Clip.ClipAspect = xml_clip->FirstChildElement("aspect")->FloatText();

    auto xml_camera = doc.FirstChildElement("world")->FirstChildElement("camera");
    config->Camera.Position = GetXYZ(xml_camera->FirstChildElement("position"));
    config->Camera.Target = GetXYZ(xml_camera->FirstChildElement("target"));
    config->Camera.Up = GetXYZ(xml_camera->FirstChildElement("up"));

    auto xml_lights = doc.FirstChildElement("world")->FirstChildElement("lights");
    for (auto xml_light = xml_lights->FirstChildElement("light"); xml_light != nullptr; xml_light = xml_light->NextSiblingElement("light"))
    {
        PointLightConfig light;
        light.Color = GetRGB(xml_light->FirstChildElement("color"));
        light.Position = GetXYZ(xml_light->FirstChildElement("position"));
        light.AmbientColor = GetRGB(xml_light->FirstChildElement("ambient")->FirstChildElement("color"));
        light.DiffuseColor = GetRGB(xml_light->FirstChildElement("diffuse")->FirstChildElement("color"));
        light.SpecularColor = GetRGB(xml_light->FirstChildElement("specular")->FirstChildElement("color"));
        light.Attenuation.Constant = xml_light->FirstChildElement("attenuation")->FirstChildElement("constant")->FloatText();
        light.Attenuation.Linear = xml_light->FirstChildElement("attenuation")->FirstChildElement("linear")->FloatText();
        light.Attenuation.Exp = xml_light->FirstChildElement("attenuation")->FirstChildElement("exp")->FloatText();

        config->PointLights.push_back(light);
    }

    auto models = doc.FirstChildElement("world")->FirstChildElement("models");
    for (auto model = models->FirstChildElement("model"); model != nullptr; model = model->NextSiblingElement())
    {
        ModelCoinfig modelConfig;
        modelConfig.Name = model->FirstChildElement("name")->GetText();

        auto mesh = model->FirstChildElement("mesh");
        modelConfig.Mesh.Name = mesh->Attribute("name");
        modelConfig.Mesh.File = mesh->FirstChildElement("file")->GetText();

        auto xml_materials = model->FirstChildElement("material");
        modelConfig.Material.AmbientColor = GetRGB(xml_materials->FirstChildElement("ambient"));
        modelConfig.Material.DiffuseColor = GetRGB(xml_materials->FirstChildElement("diffuse"));
        modelConfig.Material.SpecularColor = GetRGB(xml_materials->FirstChildElement("specular"));
        modelConfig.Material.Shininess = xml_materials->FirstChildElement("shininess")->FloatText();

        auto xml_shader = model->FirstChildElement("shader");
        modelConfig.ShaderVertFile = xml_shader->FirstChildElement("vert")->GetText();
        modelConfig.ShaderFragFile = xml_shader->FirstChildElement("frag")->GetText();

        modelConfig.Effect = "light";

        modelConfig.Position = GetXYZ(model->FirstChildElement("position"));
        modelConfig.Rotation = model->FirstChildElement("rotation")->FloatText();
        modelConfig.Scale = GetXYZ(model->FirstChildElement("scale"));

        config->Models.push_back(modelConfig);
    }
    return config;
}
