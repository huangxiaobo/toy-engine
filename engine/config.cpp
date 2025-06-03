#include "config.h"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <vector>

Config::Config() {
}

Config::~Config() {
}


Config *Config::LoadFromYaml(const std::string &filename) {
    auto *config = new Config();
    try {
        YAML::Node yaml_config = YAML::LoadFile(filename);

        const YAML::Node &world_config = yaml_config["world"];
        // windows
        const YAML::Node &window = world_config["window"];
        config->Window.WindowWidth = window["width"].as<int>();
        config->Window.WindowHeight = window["height"].as<int>();

        // clip
        const YAML::Node &clip = world_config["clip"];
        config->Clip.ClipNear = clip["near"].as<float>();
        config->Clip.ClipFar = clip["far"].as<float>();
        config->Clip.ClipFov = clip["fov"].as<float>();
        config->Clip.ClipAspect = clip["aspect"].as<float>();
        // camera
        const YAML::Node &camera = world_config["camera"];
        config->Camera.Position = glm::vec3(
            camera["position"]["x"].as<float>(),
            camera["position"]["y"].as<float>(),
            camera["position"]["z"].as<float>()
        );
        config->Camera.Target = glm::vec3(
            camera["target"]["x"].as<float>(),
            camera["target"]["y"].as<float>(),
            camera["target"]["z"].as<float>()
        );

        config->Camera.Up = glm::vec3(
            camera["up"]["x"].as<float>(),
            camera["up"]["y"].as<float>(),
            camera["up"]["z"].as<float>()
        );


        // lights
        const YAML::Node &ligth_nodes = world_config["lights"];
        for (auto &i: ligth_nodes) {
            const YAML::Node &light_node = i;
            PointLightConfig lightConfig;
            lightConfig.Color = glm::vec3(
                light_node["color"]["r"].as<float>(),
                light_node["color"]["g"].as<float>(),
                light_node["color"]["b"].as<float>()
            );
            lightConfig.Position = glm::vec3(
                light_node["position"]["x"].as<float>(),
                light_node["position"]["y"].as<float>(),
                light_node["position"]["z"].as<float>()
            );
            lightConfig.AmbientColor = glm::vec3(
                light_node["ambient"]["color"]["r"].as<float>(),
                light_node["ambient"]["color"]["g"].as<float>(),
                light_node["ambient"]["color"]["b"].as<float>()
            );
            lightConfig.DiffuseColor = glm::vec3(
                light_node["diffuse"]["color"]["r"].as<float>(),
                light_node["diffuse"]["color"]["g"].as<float>(),
                light_node["diffuse"]["color"]["b"].as<float>()
            );
            lightConfig.SpecularColor = glm::vec3(
                light_node["specular"]["color"]["r"].as<float>(),
                light_node["specular"]["color"]["g"].as<float>(),
                light_node["specular"]["color"]["b"].as<float>()
            );

            lightConfig.Attenuation.Constant = light_node["attenuation"]["constant"].as<float>();
            lightConfig.Attenuation.Linear = light_node["attenuation"]["linear"].as<float>();
            lightConfig.Attenuation.Exp = light_node["attenuation"]["exp"].as<float>();

            config->PointLights.push_back(lightConfig);
        }

        // models
        const YAML::Node &model_nodes = world_config["models"];
        for (const auto &i: model_nodes) {
            const YAML::Node &model_node = i;

            ModelCoinfig modelConfig;
            modelConfig.Name = model_node["name"].as<std::string>();

            modelConfig.Mesh.Name = model_node["mesh"]["name"].as<std::string>();
            modelConfig.Mesh.File = model_node["mesh"]["file"].as<std::string>();

            modelConfig.Material.AmbientColor = glm::vec3(
                model_node["material"]["ambient"]["r"].as<float>(),
                model_node["material"]["ambient"]["g"].as<float>(),
                model_node["material"]["ambient"]["b"].as<float>()
            );
            modelConfig.Material.DiffuseColor = glm::vec3(
                model_node["material"]["diffuse"]["r"].as<float>(),
                model_node["material"]["diffuse"]["g"].as<float>(),
                model_node["material"]["diffuse"]["b"].as<float>()
            );
            modelConfig.Material.SpecularColor = glm::vec3(
                model_node["material"]["specular"]["r"].as<float>(),
                model_node["material"]["specular"]["g"].as<float>(),
                model_node["material"]["specular"]["b"].as<float>()
            );
            modelConfig.Material.Shininess = model_node["material"]["shininess"].as<float>();


            modelConfig.ShaderVertFile = model_node["shader"]["vert"].as<std::string>();
            modelConfig.ShaderFragFile = model_node["shader"]["frag"].as<std::string>();

            modelConfig.Effect = "light";

            modelConfig.Position = glm::vec3(
                model_node["position"]["x"].as<float>(),
                model_node["position"]["y"].as<float>(),
                model_node["position"]["z"].as<float>()
            );
            modelConfig.Rotation = model_node["rotation"].as<float>();
            modelConfig.Scale = glm::vec3(
                model_node["scale"]["x"].as<float>(),
                model_node["scale"]["y"].as<float>(),
                model_node["scale"]["z"].as<float>()
            );


            config->Models.push_back(modelConfig);
        }
    } catch (const YAML::BadFile &e) {
        std::cerr << "Error loading world from yaml file: " << e.what() << std::endl;
    }
    return config;
}
