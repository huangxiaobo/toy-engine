#include <glad/gl.h>
#include "globals.h"
#include "config.h"
#include "renderer.h"
#include <iostream>
#include <string>
#include <format>

#include "mesh/mesh.h"
#include "technique/technique.h"
#include "technique/technique_light.h"
#include "model/model.h"
#include "axis/axis.h"
#include "utils/utils.h"
#include "light/light.h"
#include "material/material.h"
#include "camera/camera.h"
#include "fps/fps.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

Renderer::Renderer() {
}

Renderer::~Renderer() {
    if (m_camera != nullptr) {
        delete m_camera;
        m_camera = nullptr;
    }
    if (m_axis != nullptr) {
        delete m_axis;
        m_axis = nullptr;
    }
    while (!m_models.empty()) {
        for (auto model: m_models) {
            delete model;
        }
        m_models.clear();
    }
    while (!m_lights.empty()) {
        for (auto light: m_lights) {
            delete light;
        }
        m_lights.clear();
    }
    if (m_fps_counter != nullptr) {
        delete m_fps_counter;
        m_fps_counter = nullptr;
    }
}

void Renderer::init(int w, int h) {
    // glad 初始化
    if (!gladLoaderLoadGL()) {
        std::cout << ("glad init failed!") << std::endl;
        return;
    }

    const char *version = (const char *) glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;

    glEnable(GL_DEPTH_TEST);
    // 禁用了程序点大小模式，使用命令指定派生点大小。
    // 如果要启用程序点大小模式，则需要在shader中设置gl_PointSize
    glDisable(GL_PROGRAM_POINT_SIZE);
    // 以填充模式绘制前后
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // 设置 OpenGL 只绘制正面 , 不绘制背面
    // glEnable(GL_CULL_FACE);
    // 设置顺时针方向 CW : Clock Wind 顺时针方向, 默认是 GL_CCW : Counter Clock Wind 逆时针方向
    // glFrontFace(GL_CW);

    width = w;
    height = h;

    m_fps_counter = new FPSCounter();

    m_projection_matrix = glm::mat4(1.0f);
    m_view_matrix = glm::mat4(1.0f); //  默认生成的是一个单位矩阵（对角线上的元素为1）
    m_model_matrix = glm::mat4(1.0f); // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！
    m_eye_pos = glm::vec3(0, 0, 0);
    calculateProjectMatrix(w, h);

    // 创建坐标系模型
    auto axis_model = new Model("axis");
    auto axis_mesh = Mesh::CreateAxisMesh();
    axis_model->SetMesh(axis_mesh);

    Technique *axis_effect = new Technique("axis",
                                           "./resource/shader/axis.vert",
                                           "./resource/shader/axis.frag");

    for (auto m: axis_mesh) {
        m->SetEffect(axis_effect);
    }

    axis_model->SetTranslate(glm::vec3(0, 0.01, 0));
    m_axis = new Axis();
    m_axis->SetModel(axis_model);

    // Create Plane
    vector<Mesh *> plane_mesh = Mesh::CreatePlaneMesh();
    Technique *plane_effect = new Technique("plane",
                                            "./resource/shader/light.vert",
                                            "./resource/shader/light.frag");

    Model *plane = new Model("plane");
    plane->SetScale(glm::vec3(5.0f, 5.0f, 5.0f));
    plane->SetMesh(plane_mesh);
    for (auto m: plane_mesh) {
        m->SetEffect(plane_effect);
    }

    m_models.push_back(plane);

    // Create Ground
    vector<Mesh *> ground_mesh = Mesh::CreateGroundMesh();
    Technique *ground_effect = new Technique("ground",
                                             "./resource/shader/light.vert",
                                             "./resource/shader/light.frag");

    m_ground = new Model("ground");
    m_ground->SetScale(glm::vec3(2.1f, 2.0f, 2.1f));
    m_ground->SetMesh(ground_mesh);
    for (auto m: ground_mesh) {
        m->SetEffect(ground_effect);
    }

    m_camera = new Camera(
        gConfig->Camera.Position,
        gConfig->Camera.Target,
        gConfig->Camera.Up);

    int i = 0;
    for (auto lightConfig: gConfig->PointLights) {
        auto light = new PointLight(std::format("light-{}", i + 1));
        light->Color = lightConfig.Color;
        light->Position = lightConfig.Position;
        light->AmbientColor = lightConfig.AmbientColor;
        light->DiffuseColor = lightConfig.DiffuseColor;
        light->SpecularColor = lightConfig.SpecularColor;
        light->Attenuation.Constant = lightConfig.Attenuation.Constant;
        light->Attenuation.Linear = lightConfig.Attenuation.Linear;
        light->Attenuation.Exp = lightConfig.Attenuation.Exp;
        m_lights.push_back(light);

        // 创建光源模型
        auto model = new Model(std::format("light-{}", i++));
        // auto mesh = Mesh::CreatePointMesh(light->Position, light->Color);
        auto mesh = Mesh::CreateIcosphereMesh(5, light->Position, light->Color);
        model->SetMesh(mesh);
        model->SetScale(glm::vec3(0.5f));

        Technique *effect = new Technique(
            "onlycolor",
            "./resource/shader/onlycolor.vert",
            "./resource/shader/onlycolor.frag"
        );

        for (auto m: mesh) {
            m->SetEffect(effect);
        }

        m_light_models[light->GetUUID()] = model;
        std::cout << "Setup light finish" << std::endl;
    }
    std::cout << "Setup lights finish" << std::endl;

    for (auto modelConfig: gConfig->Models) {
        std::cout << "model name : " << modelConfig.Name << std::endl;
        std::cout << "mesh name  : " << modelConfig.Mesh.Name << std::endl;
        std::cout << "mesh file  : " << modelConfig.Mesh.File << std::endl;
        if (modelConfig.Mesh.File.length() == 0) {
            continue;
        }
        auto model_obj = new Model(modelConfig.Name);
        model_obj->LoadModel(modelConfig.Mesh.File);

        Material *material = new Material();
        material->AmbientColor = modelConfig.Material.AmbientColor;
        material->DiffuseColor = modelConfig.Material.DiffuseColor;
        material->SpecularColor = modelConfig.Material.SpecularColor;
        material->Shininess = modelConfig.Material.Shininess;

        TechniqueLight *effect = new TechniqueLight(
            "default",
            modelConfig.ShaderVertFile,
            modelConfig.ShaderFragFile
        );
        effect->SetMaterial(material);
        effect->SetLights(m_lights);
        for (auto m: model_obj->GetMeshes()) {
            m->SetEffect(effect);
        }

        model_obj->SetScale(modelConfig.Scale);
        model_obj->SetTranslate(modelConfig.Position);
        model_obj->SetRotate(modelConfig.Rotation);

        m_models.push_back(model_obj);
    }
    std::cout << "Setup world finish" << std::endl;
}

void Renderer::draw(long long elapsed) {
    // 绘制帧数量加1
    m_fps_counter->Add();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_view_matrix = m_camera->GetViewMatrix(); // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！
    m_eye_pos = m_camera->GetEyePosition();

    // 绘制坐标轴
    m_axis->GetModel()->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    // 绘制地面网格
    m_ground->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    // 绘制光源模式
    // 更新灯光
    for (auto light: m_lights) {
        if (light->GetLightType() == LightTypePoint) {
            auto point_light = (PointLight *) light;
            if (m_light_models.contains(point_light->GetUUID()) == 0) {
                continue;
            }
            auto model = m_light_models.at(point_light->GetUUID());
            if (model != nullptr) {
                model->SetTranslate(point_light->Position);
                for (const auto &m: model->GetMeshes()) {
                    auto tech = m->GetEffect();
                    tech->Enable();
                    tech->SetUniform("color", point_light->Color);
                }
            }
        }
    }
    for (const auto &kv: m_light_models) {
        auto m = kv.second;
        m->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }
    // 绘制模式
    for (const auto &m: m_models) {
        m->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }
}

void Renderer::resize(int w, int h) {
    width = w;
    height = h;
    calculateProjectMatrix(w, h);
    glViewport(0, 0, w, h);
}

void Renderer::update(long long elapsed) {
    m_eye_pos = m_camera->GetEyePosition();

    // 更新灯光
    for (auto light: m_lights) {
        if (light->GetLightType() == LightTypePoint) {
            auto point_light = (PointLight *) light;
            point_light->Position = glm::vec3(
                glm::rotate(
                    glm::radians(0.5f),
                    glm::vec3(0, 1, 0)
                ) * glm::vec4(point_light->Position, 1.0f)
            );
        }
    }
}

void Renderer::LoadWorldFromFile(const string &filename) {
    try {
        YAML::Node yaml_config = YAML::LoadFile(filename);

        const YAML::Node &world_config = yaml_config["world"];
        // windows
        const YAML::Node &window = world_config["window"];
        gConfig->Window.WindowWidth = window["width"].as<int>();
        gConfig->Window.WindowHeight = window["height"].as<int>();

        // clip
        const YAML::Node &clip = world_config["clip"];
        gConfig->Clip.ClipNear = clip["near"].as<float>();
        gConfig->Clip.ClipFar = clip["far"].as<float>();
        gConfig->Clip.ClipFov = clip["fov"].as<float>();
        gConfig->Clip.ClipAspect = clip["aspect"].as<float>();
        std::cout << 1111 << std::endl;
        // camera
        if (m_camera != nullptr) {
            delete m_camera;
        }
        m_camera = LoadCameraFromYaml(world_config["camera"]);


        // lights
        for (auto &m_light: m_lights) {
            delete m_light;
        }
        m_lights.clear();
        for (auto &m: m_light_models) {
            delete m.second;
        }
        m_light_models.clear();

        const YAML::Node &light_nodes = world_config["lights"];
        size_t index = 0;
        for (auto &i: light_nodes) {
            const YAML::Node &light_node = i;
            auto light = LoadLightFromYaml(light_node, ++index);
            auto pointLight = (PointLight *) light;
            m_lights.push_back(light);

            // 创建光源模型
            auto model = new Model(std::format("light-{}", index));
            // auto mesh = Mesh::CreatePointMesh(light->Position, light->Color);
            auto mesh = Mesh::CreateIcosphereMesh(5, pointLight->Position, pointLight->Color);
            model->SetMesh(mesh);
            model->SetScale(glm::vec3(0.5f));

            Technique *effect = new Technique(
                "onlycolor",
                "./resource/shader/onlycolor.vert",
                "./resource/shader/onlycolor.frag"
            );

            for (auto m: mesh) {
                m->SetEffect(effect);
            }

            m_light_models[light->GetUUID()] = model;
            std::cout << "Setup light finish" << std::endl;
        }

        // models
        for (auto &m: m_models) {
            delete m;
        }
        m_models.clear();
        const YAML::Node &model_nodes = world_config["models"];
        for (const auto &i: model_nodes) {
            const YAML::Node &model_node = i;
            m_models.push_back(LoadModelFromYaml(model_node, ++index));
        }
    } catch (const YAML::BadFile &e) {
        Utils::PrintStackTrace();
        std::cerr << "Error loading world from yaml file: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        Utils::PrintStackTrace();
    }
    std::cout << "load world from yaml file done" << std::endl;
}

Camera *Renderer::LoadCameraFromYaml(const YAML::Node &camera_node) {
    auto cameraPosition = glm::vec3(
        camera_node["position"]["x"].as<float>(),
        camera_node["position"]["y"].as<float>(),
        camera_node["position"]["z"].as<float>()
    );
    auto cameraTarget = glm::vec3(
        camera_node["target"]["x"].as<float>(),
        camera_node["target"]["y"].as<float>(),
        camera_node["target"]["z"].as<float>()
    );

    auto cameraUp = glm::vec3(
        camera_node["up"]["x"].as<float>(),
        camera_node["up"]["y"].as<float>(),
        camera_node["up"]["z"].as<float>()
    );
    return new Camera(
        cameraPosition,
        cameraTarget,
        cameraUp
    );
}

Model *Renderer::GetModel(string name) {
    for (auto model: m_models) {
        if (model->GetName() == name) {
            return model;
        }
    }
    return nullptr;
}

Model *Renderer::GetModelByUUID(string uuid) {
    for (auto model: m_models) {
        if (model->GetUUID() == uuid) {
            return model;
        }
    }
    return nullptr;
}

Light *Renderer::GetLightByUUID(std::string uuid) {
    for (auto light: m_lights) {
        if (light->GetUUID() == uuid) {
            return light;
        }
    }
    return nullptr;
}

float Renderer::GetFPS() {
    return m_fps_counter->GetFPS();
}

void Renderer::calculateProjectMatrix(int w, int h) {
    float fov = gConfig->Clip.ClipFov; // 视野角度
    float aspectRatio = (float) w / (float) (1 * h); // 宽高比
    float nearPlane = gConfig->Clip.ClipNear; // 近平面距离
    float farPlane = gConfig->Clip.ClipFar; // 远平面距离
    m_projection_matrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane); // 透视
}


Light *Renderer::LoadLightFromYaml(const YAML::Node &light_node, size_t index) {
    auto lightColor = glm::vec3(
        light_node["color"]["r"].as<float>(),
        light_node["color"]["g"].as<float>(),
        light_node["color"]["b"].as<float>()
    );
    auto lightPosition = glm::vec3(
        light_node["position"]["x"].as<float>(),
        light_node["position"]["y"].as<float>(),
        light_node["position"]["z"].as<float>()
    );
    auto lightAmbientColor = glm::vec3(
        light_node["ambient"]["color"]["r"].as<float>(),
        light_node["ambient"]["color"]["g"].as<float>(),
        light_node["ambient"]["color"]["b"].as<float>()
    );
    auto lightDiffuseColor = glm::vec3(
        light_node["diffuse"]["color"]["r"].as<float>(),
        light_node["diffuse"]["color"]["g"].as<float>(),
        light_node["diffuse"]["color"]["b"].as<float>()
    );
    auto lightSpecularColor = glm::vec3(
        light_node["specular"]["color"]["r"].as<float>(),
        light_node["specular"]["color"]["g"].as<float>(),
        light_node["specular"]["color"]["b"].as<float>()
    );

    auto lightAttenuationConstant = light_node["attenuation"]["constant"].as<float>();
    auto lightAttenuationLinear = light_node["attenuation"]["linear"].as<float>();
    auto lightAttenuationExp = light_node["attenuation"]["exp"].as<float>();

    auto light = new PointLight(std::format("light-{}", index));
    light->Color = lightColor;
    light->Position = lightPosition;
    light->AmbientColor = lightAmbientColor;
    light->DiffuseColor = lightDiffuseColor;
    light->SpecularColor = lightSpecularColor;
    light->Attenuation.Constant = lightAttenuationConstant;
    light->Attenuation.Linear = lightAttenuationLinear;
    light->Attenuation.Exp = lightAttenuationExp;
    return light;
}

Model *Renderer::LoadModelFromYaml(const YAML::Node &model_node, size_t index) {
    auto modelName = model_node["name"].as<std::string>();

    auto modelMeshName = model_node["mesh"]["name"].as<std::string>();
    auto modelMeshFile = model_node["mesh"]["file"].as<std::string>();


    auto modelShaderVertFile = model_node["shader"]["vert"].as<std::string>();
    auto modelShaderFragFile = model_node["shader"]["frag"].as<std::string>();

    auto modelEffect = "light";

    auto modelPosition = glm::vec3(
        model_node["position"]["x"].as<float>(),
        model_node["position"]["y"].as<float>(),
        model_node["position"]["z"].as<float>()
    );
    auto modelRotation = model_node["rotation"].as<float>();
    auto modelScale = glm::vec3(
        model_node["scale"]["x"].as<float>(),
        model_node["scale"]["y"].as<float>(),
        model_node["scale"]["z"].as<float>()
    );

    std::cout << "model name : " << modelName << std::endl;
    std::cout << "mesh name  : " << modelMeshName << std::endl;
    std::cout << "mesh file  : " << modelMeshFile << std::endl;
    if (modelMeshFile.empty()) {
        return nullptr;
    }
    auto model_obj = new Model(modelName);
    model_obj->LoadModel(modelMeshFile);


    auto *effect = new TechniqueLight(
        "default",
        modelShaderVertFile,
        modelShaderFragFile
    );
    auto material = LoadMaterialFromYaml(model_node["material"], 0);
    effect->SetMaterial(material);

    // effect->SetLights(m_lights);
    for (auto m: model_obj->GetMeshes()) {
        m->SetEffect(effect);
    }

    model_obj->SetScale(modelScale);
    model_obj->SetTranslate(modelPosition);
    model_obj->SetRotate(modelRotation);

    return model_obj;
}

Material *Renderer::LoadMaterialFromYaml(const YAML::Node &node, size_t index) {
    auto modelMaterialAmbientColor = glm::vec3(
        node["ambient"]["r"].as<float>(),
        node["ambient"]["g"].as<float>(),
        node["ambient"]["b"].as<float>()
    );
    auto modelMaterialDiffuseColor = glm::vec3(
        node["diffuse"]["r"].as<float>(),
        node["diffuse"]["g"].as<float>(),
        node["diffuse"]["b"].as<float>()
    );
    auto modelMaterialSpecularColor = glm::vec3(
        node["specular"]["r"].as<float>(),
        node["specular"]["g"].as<float>(),
        node["specular"]["b"].as<float>()
    );
    auto modelMaterialShininess = node["shininess"].as<float>();


    Material *material = new Material();
    material->AmbientColor = modelMaterialAmbientColor;
    material->DiffuseColor = modelMaterialDiffuseColor;
    material->SpecularColor = modelMaterialSpecularColor;
    material->Shininess = modelMaterialShininess;
    return material;
}

Technique *Renderer::LoadTechniqueFromYaml(const YAML::Node &node, size_t index) {
}
