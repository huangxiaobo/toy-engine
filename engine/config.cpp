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
        
        // cameras - 支持多个摄像机
        const YAML::Node &camera_nodes = world_config["cameras"];
        if (camera_nodes && camera_nodes.IsSequence()) {
            for (const auto &camera_node : camera_nodes) {
                CameraConfig cameraConfig;
                // 读取名称（可选）
                if (camera_node["name"]) {
                    cameraConfig.Name = camera_node["name"].as<std::string>();
                } else {
                    cameraConfig.Name = "Camera " + std::to_string(config->Cameras.size());
                }
                cameraConfig.Position = glm::vec3(
                    camera_node["position"]["x"].as<float>(),
                    camera_node["position"]["y"].as<float>(),
                    camera_node["position"]["z"].as<float>()
                );
                cameraConfig.Target = glm::vec3(
                    camera_node["target"]["x"].as<float>(),
                    camera_node["target"]["y"].as<float>(),
                    camera_node["target"]["z"].as<float>()
                );
                cameraConfig.Up = glm::vec3(
                    camera_node["up"]["x"].as<float>(),
                    camera_node["up"]["y"].as<float>(),
                    camera_node["up"]["z"].as<float>()
                );
                config->Cameras.push_back(cameraConfig);
            }
        } else {
            // 兼容旧格式：单个camera配置
            const YAML::Node &camera = world_config["camera"];
            if (camera) {
                CameraConfig cameraConfig;
                cameraConfig.Name = "Main Camera";
                cameraConfig.Position = glm::vec3(
                    camera["position"]["x"].as<float>(),
                    camera["position"]["y"].as<float>(),
                    camera["position"]["z"].as<float>()
                );
                cameraConfig.Target = glm::vec3(
                    camera["target"]["x"].as<float>(),
                    camera["target"]["y"].as<float>(),
                    camera["target"]["z"].as<float>()
                );
                cameraConfig.Up = glm::vec3(
                    camera["up"]["x"].as<float>(),
                    camera["up"]["y"].as<float>(),
                    camera["up"]["z"].as<float>()
                );
                config->Cameras.push_back(cameraConfig);
            }
        }


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

        // particles
        const YAML::Node &particle_nodes = world_config["particles"];
        if (particle_nodes && particle_nodes.IsSequence()) {
            for (const auto &particle_node : particle_nodes) {
                ParticleConfig particleConfig;
                
                particleConfig.Name = particle_node["name"].as<std::string>();
                particleConfig.Id = particle_node["id"].as<std::string>();
                
                particleConfig.Position = glm::vec3(
                    particle_node["position"]["x"].as<float>(),
                    particle_node["position"]["y"].as<float>(),
                    particle_node["position"]["z"].as<float>()
                );
                
                // 可选属性，有默认值
                particleConfig.EmitRate = particle_node["emit_rate"] ? particle_node["emit_rate"].as<float>() : 50.0f;
                particleConfig.MaxParticles = particle_node["max_particles"] ? particle_node["max_particles"].as<int>() : 500;
                
                particleConfig.MinLife = particle_node["min_life"] ? particle_node["min_life"].as<float>() : 1.0f;
                particleConfig.MaxLife = particle_node["max_life"] ? particle_node["max_life"].as<float>() : 2.0f;
                
                particleConfig.MinSize = particle_node["min_size"] ? particle_node["min_size"].as<float>() : 0.05f;
                particleConfig.MaxSize = particle_node["max_size"] ? particle_node["max_size"].as<float>() : 0.15f;
                
                if (particle_node["min_velocity"]) {
                    particleConfig.MinVelocity = glm::vec3(
                        particle_node["min_velocity"]["x"].as<float>(),
                        particle_node["min_velocity"]["y"].as<float>(),
                        particle_node["min_velocity"]["z"].as<float>()
                    );
                } else {
                    particleConfig.MinVelocity = glm::vec3(-0.5f, 1.0f, -0.5f);
                }
                
                if (particle_node["max_velocity"]) {
                    particleConfig.MaxVelocity = glm::vec3(
                        particle_node["max_velocity"]["x"].as<float>(),
                        particle_node["max_velocity"]["y"].as<float>(),
                        particle_node["max_velocity"]["z"].as<float>()
                    );
                } else {
                    particleConfig.MaxVelocity = glm::vec3(0.5f, 3.0f, 0.5f);
                }
                
                if (particle_node["min_color"]) {
                    particleConfig.MinColor = glm::vec3(
                        particle_node["min_color"]["r"].as<float>(),
                        particle_node["min_color"]["g"].as<float>(),
                        particle_node["min_color"]["b"].as<float>()
                    );
                } else {
                    particleConfig.MinColor = glm::vec3(1.0f, 0.6f, 0.0f);
                }
                
                if (particle_node["max_color"]) {
                    particleConfig.MaxColor = glm::vec3(
                        particle_node["max_color"]["r"].as<float>(),
                        particle_node["max_color"]["g"].as<float>(),
                        particle_node["max_color"]["b"].as<float>()
                    );
                } else {
                    particleConfig.MaxColor = glm::vec3(1.0f, 1.0f, 0.2f);
                }
                
                if (particle_node["min_color_end"]) {
                    particleConfig.MinColorEnd = glm::vec3(
                        particle_node["min_color_end"]["r"].as<float>(),
                        particle_node["min_color_end"]["g"].as<float>(),
                        particle_node["min_color_end"]["b"].as<float>()
                    );
                } else {
                    particleConfig.MinColorEnd = glm::vec3(1.0f, 0.0f, 0.0f);
                }
                
                if (particle_node["max_color_end"]) {
                    particleConfig.MaxColorEnd = glm::vec3(
                        particle_node["max_color_end"]["r"].as<float>(),
                        particle_node["max_color_end"]["g"].as<float>(),
                        particle_node["max_color_end"]["b"].as<float>()
                    );
                } else {
                    particleConfig.MaxColorEnd = glm::vec3(0.8f, 0.2f, 0.0f);
                }
                
                particleConfig.MinSizeEnd = particle_node["min_size_end"] ? particle_node["min_size_end"].as<float>() : 0.0f;
                particleConfig.MaxSizeEnd = particle_node["max_size_end"] ? particle_node["max_size_end"].as<float>() : 0.02f;
                
                if (particle_node["gravity"]) {
                    particleConfig.Gravity = glm::vec3(
                        particle_node["gravity"]["x"].as<float>(),
                        particle_node["gravity"]["y"].as<float>(),
                        particle_node["gravity"]["z"].as<float>()
                    );
                } else {
                    particleConfig.Gravity = glm::vec3(0.0f, -2.0f, 0.0f);
                }
                
                particleConfig.Drag = particle_node["drag"] ? particle_node["drag"].as<float>() : 0.98f;
                
                config->Particles.push_back(particleConfig);
            }
        }

        // sky_dome
        const YAML::Node &sky_dome_node = world_config["sky_dome"];
        if (sky_dome_node) {
            if (sky_dome_node["radius"]) {
                config->SkyDome.Radius = sky_dome_node["radius"].as<float>();
            }
            if (sky_dome_node["sectors"]) {
                config->SkyDome.Sectors = sky_dome_node["sectors"].as<int>();
            }
            if (sky_dome_node["stacks"]) {
                config->SkyDome.Stacks = sky_dome_node["stacks"].as<int>();
            }
            if (sky_dome_node["horizon_color"]) {
                config->SkyDome.HorizonColor = glm::vec3(
                    sky_dome_node["horizon_color"]["r"].as<float>(),
                    sky_dome_node["horizon_color"]["g"].as<float>(),
                    sky_dome_node["horizon_color"]["b"].as<float>()
                );
            }
            if (sky_dome_node["zenith_color"]) {
                config->SkyDome.ZenithColor = glm::vec3(
                    sky_dome_node["zenith_color"]["r"].as<float>(),
                    sky_dome_node["zenith_color"]["g"].as<float>(),
                    sky_dome_node["zenith_color"]["b"].as<float>()
                );
            }
        }

        // terrain（程序化LOD地形）
        // 所有字段均可选，未配置时使用 TerrainConfigCfg 的默认值
        const YAML::Node &terrain_node = world_config["terrain"];
        if (terrain_node) {
            if (terrain_node["chunk-size"]) {
                config->Terrain.ChunkSize = terrain_node["chunk-size"].as<float>();
            }
            if (terrain_node["base-resolution"]) {
                config->Terrain.BaseResolution = terrain_node["base-resolution"].as<int>();
            }
            if (terrain_node["render-distance"]) {
                config->Terrain.RenderDistance = terrain_node["render-distance"].as<int>();
            }
            if (terrain_node["unload-distance"]) {
                config->Terrain.UnloadDistance = terrain_node["unload-distance"].as<int>();
            }
            if (terrain_node["height-scale"]) {
                config->Terrain.HeightScale = terrain_node["height-scale"].as<float>();
            }
            if (terrain_node["noise-seed"]) {
                config->Terrain.NoiseSeed = terrain_node["noise-seed"].as<unsigned int>();
            }
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
