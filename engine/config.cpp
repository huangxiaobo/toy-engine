#include "config.h"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <vector>

Config::Config() {
}

Config::~Config() {
}


std::unique_ptr<Config> Config::LoadFromYaml(const std::string &filename) {
    auto config = std::make_unique<Config>();
    try {
        YAML::Node yaml_config = YAML::LoadFile(filename);

        const YAML::Node &world_config = yaml_config["world"];
        // windows
        const YAML::Node &window = world_config["window"];
        config->m_window.m_window_width = window["width"].as<int>();
        config->m_window.m_window_height = window["height"].as<int>();

        // clip
        const YAML::Node &clip = world_config["clip"];
        config->m_clip.m_clip_near = clip["near"].as<float>();
        config->m_clip.m_clip_far = clip["far"].as<float>();
        config->m_clip.m_clip_fov = clip["fov"].as<float>();
        config->m_clip.m_clip_aspect = clip["aspect"].as<float>();
        
        // cameras - 支持多个摄像机
        const YAML::Node &camera_nodes = world_config["cameras"];
        if (camera_nodes && camera_nodes.IsSequence()) {
            for (const auto &camera_node : camera_nodes) {
                CameraConfig cameraConfig;
                // 读取名称（可选）
                if (camera_node["name"]) {
                    cameraConfig.m_name = camera_node["name"].as<std::string>();
                } else {
                    cameraConfig.m_name = "Camera " + std::to_string(config->m_cameras.size());
                }
                cameraConfig.m_position = glm::vec3(
                    camera_node["position"]["x"].as<float>(),
                    camera_node["position"]["y"].as<float>(),
                    camera_node["position"]["z"].as<float>()
                );
                cameraConfig.m_target = glm::vec3(
                    camera_node["target"]["x"].as<float>(),
                    camera_node["target"]["y"].as<float>(),
                    camera_node["target"]["z"].as<float>()
                );
                cameraConfig.m_up = glm::vec3(
                    camera_node["up"]["x"].as<float>(),
                    camera_node["up"]["y"].as<float>(),
                    camera_node["up"]["z"].as<float>()
                );
                // 投影模式（可选）：perspective 透视 / orthographic 正交，缺省透视
                if (camera_node["projection"]) {
                    std::string projection = camera_node["projection"].as<std::string>();
                    cameraConfig.m_projection = (projection == "orthographic")
                                                  ? ProjectionType::Orthographic
                                                  : ProjectionType::Perspective;
                }
                config->m_cameras.push_back(cameraConfig);
            }
        } else {
            // 兼容旧格式：单个camera配置
            const YAML::Node &camera = world_config["camera"];
            if (camera) {
                CameraConfig cameraConfig;
                cameraConfig.m_name = "Main Camera";
                cameraConfig.m_position = glm::vec3(
                    camera["position"]["x"].as<float>(),
                    camera["position"]["y"].as<float>(),
                    camera["position"]["z"].as<float>()
                );
                cameraConfig.m_target = glm::vec3(
                    camera["target"]["x"].as<float>(),
                    camera["target"]["y"].as<float>(),
                    camera["target"]["z"].as<float>()
                );
                cameraConfig.m_up = glm::vec3(
                    camera["up"]["x"].as<float>(),
                    camera["up"]["y"].as<float>(),
                    camera["up"]["z"].as<float>()
                );
                // 投影模式（可选）：perspective 透视 / orthographic 正交，缺省透视
                if (camera["projection"]) {
                    std::string projection = camera["projection"].as<std::string>();
                    cameraConfig.m_projection = (projection == "orthographic")
                                                  ? ProjectionType::Orthographic
                                                  : ProjectionType::Perspective;
                }
                config->m_cameras.push_back(cameraConfig);
            }
        }


        // lights
        const YAML::Node &ligth_nodes = world_config["lights"];
        for (auto &i: ligth_nodes) {
            const YAML::Node &light_node = i;

            // 根据 type 字段区分光源类型：0=方向光, 1=点光源, 2=聚光灯
            int lightType = light_node["type"] ? light_node["type"].as<int>() : 1;

            if (lightType == 0) {
                // 方向光（平行光）
                DirectionLightConfig lightConfig;
                lightConfig.m_name = light_node["name"] ? light_node["name"].as<std::string>() : "";
                lightConfig.m_id = light_node["id"] ? light_node["id"].as<std::string>() : "";
                lightConfig.m_enabled = light_node["enabled"] ? light_node["enabled"].as<bool>() : true;
                // 读取漫反射/环境光/镜面反射三通道强度（world.yaml 中 ambient/diffuse/specular 的 intensity 字段）
                lightConfig.m_ambient_intensity = light_node["ambient"]["intensity"] ? light_node["ambient"]["intensity"].as<float>() : 1.0f;
                lightConfig.m_diffuse_intensity = light_node["diffuse"]["intensity"] ? light_node["diffuse"]["intensity"].as<float>() : 1.0f;
                lightConfig.m_specular_intensity = light_node["specular"]["intensity"] ? light_node["specular"]["intensity"].as<float>() : 1.0f;
                lightConfig.m_direction = glm::vec3(
                    light_node["direction"]["x"].as<float>(),
                    light_node["direction"]["y"].as<float>(),
                    light_node["direction"]["z"].as<float>()
                );
                lightConfig.m_color = glm::vec3(
                    light_node["color"]["r"].as<float>(),
                    light_node["color"]["g"].as<float>(),
                    light_node["color"]["b"].as<float>()
                );
                lightConfig.m_ambient_color = glm::vec3(
                    light_node["ambient"]["color"]["r"].as<float>(),
                    light_node["ambient"]["color"]["g"].as<float>(),
                    light_node["ambient"]["color"]["b"].as<float>()
                );
                lightConfig.m_diffuse_color = glm::vec3(
                    light_node["diffuse"]["color"]["r"].as<float>(),
                    light_node["diffuse"]["color"]["g"].as<float>(),
                    light_node["diffuse"]["color"]["b"].as<float>()
                );
                lightConfig.m_specular_color = glm::vec3(
                    light_node["specular"]["color"]["r"].as<float>(),
                    light_node["specular"]["color"]["g"].as<float>(),
                    light_node["specular"]["color"]["b"].as<float>()
                );
                config->m_direction_lights.push_back(lightConfig);
            } else if (lightType == 2) {
                // 聚光灯
                SpotLightConfig lightConfig;
                lightConfig.m_name = light_node["name"] ? light_node["name"].as<std::string>() : "";
                lightConfig.m_id = light_node["id"] ? light_node["id"].as<std::string>() : "";
                lightConfig.m_enabled = light_node["enabled"] ? light_node["enabled"].as<bool>() : true;
                lightConfig.m_position = glm::vec3(
                    light_node["position"]["x"].as<float>(),
                    light_node["position"]["y"].as<float>(),
                    light_node["position"]["z"].as<float>()
                );
                lightConfig.m_direction = glm::vec3(
                    light_node["direction"]["x"].as<float>(),
                    light_node["direction"]["y"].as<float>(),
                    light_node["direction"]["z"].as<float>()
                );
                lightConfig.m_color = glm::vec3(
                    light_node["color"]["r"].as<float>(),
                    light_node["color"]["g"].as<float>(),
                    light_node["color"]["b"].as<float>()
                );
                lightConfig.m_ambient_color = glm::vec3(
                    light_node["ambient"]["color"]["r"].as<float>(),
                    light_node["ambient"]["color"]["g"].as<float>(),
                    light_node["ambient"]["color"]["b"].as<float>()
                );
                lightConfig.m_diffuse_color = glm::vec3(
                    light_node["diffuse"]["color"]["r"].as<float>(),
                    light_node["diffuse"]["color"]["g"].as<float>(),
                    light_node["diffuse"]["color"]["b"].as<float>()
                );
                lightConfig.m_specular_color = glm::vec3(
                    light_node["specular"]["color"]["r"].as<float>(),
                    light_node["specular"]["color"]["g"].as<float>(),
                    light_node["specular"]["color"]["b"].as<float>()
                );
                // 读取三通道强度（缺省时默认 1.0，避免旧配置缺失字段时强度为垃圾值/0）
                lightConfig.m_ambient_intensity = light_node["ambient"]["intensity"] ? light_node["ambient"]["intensity"].as<float>() : 1.0f;
                lightConfig.m_diffuse_intensity = light_node["diffuse"]["intensity"] ? light_node["diffuse"]["intensity"].as<float>() : 1.0f;
                lightConfig.m_specular_intensity = light_node["specular"]["intensity"] ? light_node["specular"]["intensity"].as<float>() : 1.0f;
                lightConfig.Attenuation.m_constant = light_node["attenuation"]["constant"].as<float>();
                lightConfig.Attenuation.m_linear = light_node["attenuation"]["linear"].as<float>();
                lightConfig.Attenuation.m_exp = light_node["attenuation"]["exp"].as<float>();
                lightConfig.m_cutoff = light_node["cutoff"].as<float>();
                lightConfig.m_outer_cutoff = light_node["outer_cutoff"].as<float>();
                config->m_spot_lights.push_back(lightConfig);
            } else {
                // 默认：点光源（兼容旧格式）
                PointLightConfig lightConfig;
                lightConfig.m_name = light_node["name"] ? light_node["name"].as<std::string>() : "";
                lightConfig.m_id = light_node["id"] ? light_node["id"].as<std::string>() : "";
                lightConfig.m_enabled = light_node["enabled"] ? light_node["enabled"].as<bool>() : true;
                lightConfig.m_color = glm::vec3(
                    light_node["color"]["r"].as<float>(),
                    light_node["color"]["g"].as<float>(),
                    light_node["color"]["b"].as<float>()
                );
                lightConfig.m_position = glm::vec3(
                    light_node["position"]["x"].as<float>(),
                    light_node["position"]["y"].as<float>(),
                    light_node["position"]["z"].as<float>()
                );
                lightConfig.m_ambient_color = glm::vec3(
                    light_node["ambient"]["color"]["r"].as<float>(),
                    light_node["ambient"]["color"]["g"].as<float>(),
                    light_node["ambient"]["color"]["b"].as<float>()
                );
                lightConfig.m_diffuse_color = glm::vec3(
                    light_node["diffuse"]["color"]["r"].as<float>(),
                    light_node["diffuse"]["color"]["g"].as<float>(),
                    light_node["diffuse"]["color"]["b"].as<float>()
                );
                lightConfig.m_specular_color = glm::vec3(
                    light_node["specular"]["color"]["r"].as<float>(),
                    light_node["specular"]["color"]["g"].as<float>(),
                    light_node["specular"]["color"]["b"].as<float>()
                );
                // 读取三通道强度（缺省时默认 1.0，避免旧配置缺失字段时强度为垃圾值/0）
                lightConfig.m_ambient_intensity = light_node["ambient"]["intensity"] ? light_node["ambient"]["intensity"].as<float>() : 1.0f;
                lightConfig.m_diffuse_intensity = light_node["diffuse"]["intensity"] ? light_node["diffuse"]["intensity"].as<float>() : 1.0f;
                lightConfig.m_specular_intensity = light_node["specular"]["intensity"] ? light_node["specular"]["intensity"].as<float>() : 1.0f;
                lightConfig.Attenuation.m_constant = light_node["attenuation"]["constant"].as<float>();
                lightConfig.Attenuation.m_linear = light_node["attenuation"]["linear"].as<float>();
                lightConfig.Attenuation.m_exp = light_node["attenuation"]["exp"].as<float>();
                config->m_point_lights.push_back(lightConfig);
            }
        }

        // particles
        const YAML::Node &particle_nodes = world_config["particles"];
        if (particle_nodes && particle_nodes.IsSequence()) {
            for (const auto &particle_node : particle_nodes) {
                ParticleConfig particleConfig;
                
                particleConfig.m_name = particle_node["name"].as<std::string>();
                particleConfig.m_id = particle_node["id"].as<std::string>();
                
                particleConfig.m_position = glm::vec3(
                    particle_node["position"]["x"].as<float>(),
                    particle_node["position"]["y"].as<float>(),
                    particle_node["position"]["z"].as<float>()
                );
                
                // 可选属性，有默认值
                particleConfig.m_emit_rate = particle_node["emit_rate"] ? particle_node["emit_rate"].as<float>() : 50.0f;
                particleConfig.m_max_particles = particle_node["max_particles"] ? particle_node["max_particles"].as<int>() : 500;
                
                particleConfig.m_min_life = particle_node["min_life"] ? particle_node["min_life"].as<float>() : 1.0f;
                particleConfig.m_max_life = particle_node["max_life"] ? particle_node["max_life"].as<float>() : 2.0f;
                
                particleConfig.m_min_size = particle_node["min_size"] ? particle_node["min_size"].as<float>() : 0.05f;
                particleConfig.m_max_size = particle_node["max_size"] ? particle_node["max_size"].as<float>() : 0.15f;
                
                if (particle_node["min_velocity"]) {
                    particleConfig.m_min_velocity = glm::vec3(
                        particle_node["min_velocity"]["x"].as<float>(),
                        particle_node["min_velocity"]["y"].as<float>(),
                        particle_node["min_velocity"]["z"].as<float>()
                    );
                } else {
                    particleConfig.m_min_velocity = glm::vec3(-0.5f, 1.0f, -0.5f);
                }
                
                if (particle_node["max_velocity"]) {
                    particleConfig.m_max_velocity = glm::vec3(
                        particle_node["max_velocity"]["x"].as<float>(),
                        particle_node["max_velocity"]["y"].as<float>(),
                        particle_node["max_velocity"]["z"].as<float>()
                    );
                } else {
                    particleConfig.m_max_velocity = glm::vec3(0.5f, 3.0f, 0.5f);
                }
                
                // 颜色不再由 YAML 配置，改由发射器内部"烟花配色表"决定（见 ParticleEmitter::RandomFireworkColor）

                particleConfig.m_min_size_end = particle_node["min_size_end"] ? particle_node["min_size_end"].as<float>() : 0.0f;
                particleConfig.m_max_size_end = particle_node["max_size_end"] ? particle_node["max_size_end"].as<float>() : 0.02f;
                
                if (particle_node["gravity"]) {
                    particleConfig.m_gravity = glm::vec3(
                        particle_node["gravity"]["x"].as<float>(),
                        particle_node["gravity"]["y"].as<float>(),
                        particle_node["gravity"]["z"].as<float>()
                    );
                } else {
                    particleConfig.m_gravity = glm::vec3(0.0f, -2.0f, 0.0f);
                }
                
                particleConfig.m_drag = particle_node["drag"] ? particle_node["drag"].as<float>() : 0.98f;
                
                config->m_particles.push_back(particleConfig);
            }
        }

        // sky_dome
        const YAML::Node &sky_dome_node = world_config["sky_dome"];
        if (sky_dome_node) {
            if (sky_dome_node["radius"]) {
                config->m_sky_dome.m_radius = sky_dome_node["radius"].as<float>();
            }
            if (sky_dome_node["sectors"]) {
                config->m_sky_dome.m_sectors = sky_dome_node["sectors"].as<int>();
            }
            if (sky_dome_node["stacks"]) {
                config->m_sky_dome.m_stacks = sky_dome_node["stacks"].as<int>();
            }
            if (sky_dome_node["horizon_color"]) {
                config->m_sky_dome.m_horizon_color = glm::vec3(
                    sky_dome_node["horizon_color"]["r"].as<float>(),
                    sky_dome_node["horizon_color"]["g"].as<float>(),
                    sky_dome_node["horizon_color"]["b"].as<float>()
                );
            }
            if (sky_dome_node["zenith_color"]) {
                config->m_sky_dome.m_zenith_color = glm::vec3(
                    sky_dome_node["zenith_color"]["r"].as<float>(),
                    sky_dome_node["zenith_color"]["g"].as<float>(),
                    sky_dome_node["zenith_color"]["b"].as<float>()
                );
            }
            if (sky_dome_node["ground_color"]) {
                config->m_sky_dome.m_ground_color = glm::vec3(
                    sky_dome_node["ground_color"]["r"].as<float>(),
                    sky_dome_node["ground_color"]["g"].as<float>(),
                    sky_dome_node["ground_color"]["b"].as<float>()
                );
            }
        }

        // terrain（程序化网格地形，无 LOD）
        // 所有字段均可选，未配置时使用 TerrainConfigCfg 的默认值
        const YAML::Node &terrain_node = world_config["terrain"];
        if (terrain_node) {
            if (terrain_node["plane-size"]) {
                config->m_terrain.m_plane_size = terrain_node["plane-size"].as<float>();
            }
            if (terrain_node["resolution"]) {
                config->m_terrain.m_resolution = terrain_node["resolution"].as<int>();
            }
            if (terrain_node["height-scale"]) {
                config->m_terrain.m_height_scale = terrain_node["height-scale"].as<float>();
            }
            if (terrain_node["noise-seed"]) {
                config->m_terrain.m_noise_seed = terrain_node["noise-seed"].as<unsigned int>();
            }
        }

        // models
        const YAML::Node &model_nodes = world_config["models"];
        for (const auto &i: model_nodes) {
            const YAML::Node &model_node = i;

            ModelConfig modelConfig;
            modelConfig.m_name = model_node["name"].as<std::string>();

            modelConfig.m_mesh.m_name = model_node["mesh"]["name"].as<std::string>();
            modelConfig.m_mesh.m_file = model_node["mesh"]["file"].as<std::string>();

            // 材质改为引用 MTL 文件：material.file 为 MTL 路径，material.m_name 为材质名
            modelConfig.m_material.m_file = model_node["material"]["file"].as<std::string>();
            modelConfig.m_material.m_name = model_node["material"]["name"].as<std::string>();


            modelConfig.m_shader_vert_file = model_node["shader"]["vert"].as<std::string>();
            modelConfig.m_shader_frag_file = model_node["shader"]["frag"].as<std::string>();

            modelConfig.m_effect = "light";

            modelConfig.m_position = glm::vec3(
                model_node["position"]["x"].as<float>(),
                model_node["position"]["y"].as<float>(),
                model_node["position"]["z"].as<float>()
            );
            // 旋转支持三种写法，按优先级覆盖：
            // 1) rotation: 0       —— 旧标量形式，只设 Y 轴
            // 2) rotation: {x,y,z} —— 映射形式，三轴
            // 3) rotation_x/y/z     —— 独立键，可单独重写某一轴
            modelConfig.m_rotation = glm::vec3(0.0f);
            const auto &rot_node = model_node["rotation"];
            if (rot_node && rot_node.IsScalar()) {
                modelConfig.m_rotation.y = rot_node.as<float>();
            } else if (rot_node && rot_node.IsMap()) {
                modelConfig.m_rotation.x = rot_node["x"].as<float>(0.0f);
                modelConfig.m_rotation.y = rot_node["y"].as<float>(0.0f);
                modelConfig.m_rotation.z = rot_node["z"].as<float>(0.0f);
            }
            if (model_node["rotation_x"]) {
                modelConfig.m_rotation.x = model_node["rotation_x"].as<float>();
            }
            if (model_node["rotation_y"]) {
                modelConfig.m_rotation.y = model_node["rotation_y"].as<float>();
            }
            if (model_node["rotation_z"]) {
                modelConfig.m_rotation.z = model_node["rotation_z"].as<float>();
            }
            modelConfig.m_scale = glm::vec3(
                model_node["scale"]["x"].as<float>(),
                model_node["scale"]["y"].as<float>(),
                model_node["scale"]["z"].as<float>()
            );


            config->m_models.push_back(modelConfig);
        }

        // animations（通用动画，绑定模型或灯光做程序化动画）
        // 各通道各自可选；yaml 只写需要动画的通道，其余保持目标原值
        const YAML::Node &animation_nodes = world_config["animations"];
        if (animation_nodes && animation_nodes.IsSequence()) {
            for (const auto &anim_node : animation_nodes) {
                AnimationConfig animConfig;
                animConfig.m_name = anim_node["name"].as<std::string>();
                // 目标对象：mode 或 light 二选一，均由渲染器按名字查找绑定
                if (anim_node["light"]) {
                    animConfig.m_light_name = anim_node["light"].as<std::string>();
                } else {
                    animConfig.m_model_name = anim_node["model"].as<std::string>();
                }
                animConfig.m_enabled = anim_node["enabled"] ? anim_node["enabled"].as<bool>() : true;

                // 位移通道：center / amplitude / frequency
                const auto &trs = anim_node["translate"];
                if (trs) {
                    animConfig.m_has_translate = true;
                    animConfig.translate_center = glm::vec3(
                        trs["center"]["x"].as<float>(),
                        trs["center"]["y"].as<float>(),
                        trs["center"]["z"].as<float>()
                    );
                    animConfig.translate_amplitude = glm::vec3(
                        trs["amplitude"]["x"].as<float>(),
                        trs["amplitude"]["y"].as<float>(),
                        trs["amplitude"]["z"].as<float>()
                    );
                    animConfig.m_translate_frequency = trs["frequency"] ? trs["frequency"].as<float>() : 1.0f;
                }

                // 缩放通道：base / amplitude / frequency
                const auto &scl = anim_node["scale"];
                if (scl) {
                    animConfig.m_has_scale = true;
                    animConfig.scale_base = glm::vec3(
                        scl["base"]["x"].as<float>(),
                        scl["base"]["y"].as<float>(),
                        scl["base"]["z"].as<float>()
                    );
                    animConfig.scale_amplitude = glm::vec3(
                        scl["amplitude"]["x"].as<float>(),
                        scl["amplitude"]["y"].as<float>(),
                        scl["amplitude"]["z"].as<float>()
                    );
                    animConfig.m_scale_frequency = scl["frequency"] ? scl["frequency"].as<float>() : 1.0f;
                }

                // 旋转通道：base / amplitude / frequency / spin
                // spin=true 时用 speed（度/秒）匀速旋转，否则正弦摆动
                const auto &rot = anim_node["rotate"];
                if (rot) {
                    animConfig.m_has_rotate = true;
                    animConfig.m_rotate_spin = rot["spin"] ? rot["spin"].as<bool>() : false;
                    animConfig.rotate_base = glm::vec3(
                        rot["base"]["x"].as<float>(),
                        rot["base"]["y"].as<float>(),
                        rot["base"]["z"].as<float>()
                    );
                    if (animConfig.m_rotate_spin) {
                        animConfig.rotate_speed = glm::vec3(
                            rot["speed"]["x"].as<float>(),
                            rot["speed"]["y"].as<float>(),
                            rot["speed"]["z"].as<float>()
                        );
                    } else {
                        animConfig.rotate_amplitude = glm::vec3(
                            rot["amplitude"]["x"].as<float>(),
                            rot["amplitude"]["y"].as<float>(),
                            rot["amplitude"]["z"].as<float>()
                        );
                        animConfig.m_rotate_frequency = rot["frequency"] ? rot["frequency"].as<float>() : 1.0f;
                    }
                }

                // 灯光颜色通道：center / amplitude / frequency（RGB 正弦振荡）
                const auto &clr = anim_node["color"];
                if (clr) {
                    animConfig.m_has_color = true;
                    animConfig.color_center = glm::vec3(
                        clr["center"]["x"].as<float>(),
                        clr["center"]["y"].as<float>(),
                        clr["center"]["z"].as<float>()
                    );
                    animConfig.color_amplitude = glm::vec3(
                        clr["amplitude"]["x"].as<float>(),
                        clr["amplitude"]["y"].as<float>(),
                        clr["amplitude"]["z"].as<float>()
                    );
                    animConfig.m_color_frequency = clr["frequency"] ? clr["frequency"].as<float>() : 1.0f;
                }

                // 灯光强度通道：center / amplitude / frequency（取 vec3.x 分量）
                const auto &intensity = anim_node["intensity"];
                if (intensity) {
                    animConfig.m_has_intensity = true;
                    animConfig.intensity_center = glm::vec3(
                        intensity["center"]["x"].as<float>(),
                        intensity["center"]["y"].as<float>(),
                        intensity["center"]["z"].as<float>()
                    );
                    animConfig.intensity_amplitude = glm::vec3(
                        intensity["amplitude"]["x"].as<float>(),
                        intensity["amplitude"]["y"].as<float>(),
                        intensity["amplitude"]["z"].as<float>()
                    );
                    animConfig.m_intensity_frequency = intensity["frequency"] ? intensity["frequency"].as<float>() : 1.0f;
                }

                // 圆周轨道通道：center / radius / frequency（绕 Y 轴画圈，x/z 半径生效）
                // 与 translate 语义互斥：轨道是位置沿圆周匀速运动，而非正弦振荡
                const auto &orbit = anim_node["orbit"];
                if (orbit) {
                    animConfig.m_has_orbit = true;
                    animConfig.orbit_center = glm::vec3(
                        orbit["center"]["x"].as<float>(),
                        orbit["center"]["y"].as<float>(),
                        orbit["center"]["z"].as<float>()
                    );
                    animConfig.orbit_radius = glm::vec3(
                        orbit["radius"]["x"].as<float>(),
                        orbit["radius"]["y"].as<float>(),
                        orbit["radius"]["z"].as<float>()
                    );
                    animConfig.m_orbit_frequency = orbit["frequency"] ? orbit["frequency"].as<float>() : 1.0f;
                }

                config->m_animations.push_back(animConfig);
            }
        }
    } catch (const YAML::BadFile &e) {
        std::cerr << "Error loading world from yaml file: " << e.what() << std::endl;
    }
    return config;
}
