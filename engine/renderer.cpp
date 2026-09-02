#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "globals.h"
#include "config.h"
#include "renderer.h"
#include <iostream>
#include <string>
#include <format>
#include <algorithm>

#include "mesh/mesh.h"
#include "technique/technique.h"
#include "technique/technique_light.h"
#include "model/model.h"
#include "axis/axis.h"
#include "terrain/terrain_manager.h"
#include "particle/particle_system.h"
#include "particle/particle_emitter.h"
#include "sky/sky_dome.h"
#include "utils/utils.h"
#include "light/light.h"
#include "material/material.h"
#include "material/mtl_parser.h"
#include "camera/camera.h"
#include "fps/fps.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

Renderer::Renderer() : m_eye_pos(0) {
}

Renderer::~Renderer() {
    if (m_axis != nullptr) {
        delete m_axis;
        m_axis = nullptr;
    }
    if (m_terrain_manager != nullptr) {
        delete m_terrain_manager;
        m_terrain_manager = nullptr;
    }
    if (m_sky_dome != nullptr) {
        delete m_sky_dome;
        m_sky_dome = nullptr;
    }
    while (!m_particle_systems.empty()) {
        for (auto ps: m_particle_systems) {
            delete ps;
        }
        m_particle_systems.clear();
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

    // 释放光源模型（m_light_models 与 light->m_model 指向同一对象，只在 map 中删一次）
    for (auto &kv: m_light_models) {
        delete kv.second;
    }
    m_light_models.clear();

    // 释放所有摄像机
    // 注意：m_camera 始终是 m_cameras 中的一个元素，这里统一删除一次即可，
    // 不能单独再删 m_camera，否则会与这里的删除发生双重释放
    for (auto cam: m_cameras) {
        delete cam;
    }
    m_cameras.clear();

    // 释放渲染器创建并拥有的着色器与材质
    for (auto tech: m_techniques) {
        delete tech;
    }
    m_techniques.clear();
    for (auto mat: m_materials) {
        delete mat;
    }
    m_materials.clear();

    // 释放地形纹理
    if (m_terrain_texture != 0) {
        glDeleteTextures(1, &m_terrain_texture);
        m_terrain_texture = 0;
    }

    // 释放模型纹理（漫反射/法线贴图等）
    if (!m_textures.empty()) {
        glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
        m_textures.clear();
    }

    if (m_fps_counter != nullptr) {
        delete m_fps_counter;
        m_fps_counter = nullptr;
    }
}

void Renderer::init(int w, int h) {
    // glad 初始化
    if (!gladLoadGL(glfwGetProcAddress)) {
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
    // 统一设置线宽，供线段/点光源模型等以线框方式绘制的对象使用(原实现在每次 Model::Draw 中重复设置)
    glLineWidth(5);
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

    // 创建屏幕空间坐标轴 gizmo（视口角落叠加的 ImGui 绘制，非 3D 网格）
    // 具体绘制见 mainwindow.cpp RenderFrame 中的 ApplyViewportAxisGizmo 调用
    m_axis = new Axis();

    // Create Plane
    vector<Mesh *> plane_mesh = Mesh::CreatePlaneMesh();
    auto *plane_effect = new Technique("plane",
                                       "./resource/shader/light.vert",
                                       "./resource/shader/light.frag");

    auto *plane = new Model("plane");
    plane->SetScale(glm::vec3(5.0f, 5.0f, 5.0f));
    plane->SetMeshes(plane_mesh);
    for (auto m: plane_mesh) {
        m->SetEffect(plane_effect);
    }
    // 平面着色器交由渲染器统一管理释放
    m_techniques.push_back(plane_effect);

    m_models.push_back(plane);

    // 创建地形管理器（固定尺寸网格平面，无 LOD）
    m_terrain_manager = new TerrainManager();
    TerrainConfig terrainConfig;
    terrainConfig.planeSize = gConfig->Terrain.PlaneSize;    // 平面尺寸 200×200 单位
    terrainConfig.resolution = gConfig->Terrain.Resolution;  // 网格分辨率
    terrainConfig.heightScale = gConfig->Terrain.HeightScale; // 最大高度（0 = 平坦）
    terrainConfig.noiseSeed = gConfig->Terrain.NoiseSeed;     // 噪声种子
    m_terrain_manager->Init(terrainConfig);

    // 创建地形着色器和材质
    auto *terrainEffect = new TechniqueLight("terrain",
                                             "./resource/shader/terrain.vert",
                                             "./resource/shader/terrain.frag");
    Material *terrainMaterial = new Material();
    terrainMaterial->AmbientColor = glm::vec3(0.3f, 0.3f, 0.3f);
    terrainMaterial->DiffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    terrainMaterial->SpecularColor = glm::vec3(0.5f, 0.5f, 0.5f);
    terrainMaterial->Shininess = 32.0f;
    terrainEffect->SetMaterial(terrainMaterial);
    terrainEffect->SetLights(m_lights);
    m_terrain_manager->SetTechnique(terrainEffect);
    // 交由渲染器统一管理地形着色器与材质的释放
    m_techniques.push_back(terrainEffect);
    m_materials.push_back(terrainMaterial);

    // 创建并绑定地形纹理
    m_terrain_texture = Utils::CreateCheckerboardTexture(512, 512, 64);
    m_terrain_manager->SetTexture(m_terrain_texture);

    m_sky_dome = new SkyDome();
    m_sky_dome->Init(
        gConfig->SkyDome.Radius,
        gConfig->SkyDome.Sectors,
        gConfig->SkyDome.Stacks
    );
    m_sky_dome->SetHorizonColor(gConfig->SkyDome.HorizonColor);
    m_sky_dome->SetZenithColor(gConfig->SkyDome.ZenithColor);

    // Create Particle Systems from config
    for (const auto &particleConfig: gConfig->Particles) {
        auto *ps = new ParticleSystem();
        ps->Init(particleConfig.Position);
        
        // 应用配置
        auto *emitter = ps->GetEmitter();
        emitter->SetMaxParticles(particleConfig.MaxParticles);
        ps->ReallocateVBO();
        emitter->EmitRate = particleConfig.EmitRate;
        emitter->MinLife = particleConfig.MinLife;
        emitter->MaxLife = particleConfig.MaxLife;
        emitter->MinSize = particleConfig.MinSize;
        emitter->MaxSize = particleConfig.MaxSize;
        emitter->MinVelocity = particleConfig.MinVelocity;
        emitter->MaxVelocity = particleConfig.MaxVelocity;
        emitter->MinColor = particleConfig.MinColor;
        emitter->MaxColor = particleConfig.MaxColor;
        emitter->MinColorEnd = particleConfig.MinColorEnd;
        emitter->MaxColorEnd = particleConfig.MaxColorEnd;
        emitter->MinSizeEnd = particleConfig.MinSizeEnd;
        emitter->MaxSizeEnd = particleConfig.MaxSizeEnd;
        emitter->Gravity = particleConfig.Gravity;
        emitter->Drag = particleConfig.Drag;
        
        m_particle_systems.push_back(ps);
    }

    for (const auto &cameraConfig: gConfig->Cameras) {
        auto camera = new Camera(
            cameraConfig.Position,
            cameraConfig.Target,
            cameraConfig.Up
        );
        camera->m_name = cameraConfig.Name;
        m_cameras.push_back(camera);
    }
    // 复用 m_cameras 中的第一个摄像机作为当前摄像机，避免重复创建造成内存泄漏
    m_camera = m_cameras[0];

    int i = 0;
    // 创建方向光（平行光）
    for (const auto &lightConfig: gConfig->DirectionLights) {
        // 名称优先使用 world.yaml 中的 name，未配置时回退为自动生成的索引名
        std::string dirName = lightConfig.Name.empty() ? std::format("dir-light-{}", i + 1) : lightConfig.Name;
        auto light = new DirectionLight(dirName);
        // 若配置了 id，则覆盖默认自动生成的 UUID 用于稳定标识
        if (!lightConfig.Id.empty()) {
            light->SetUUID(lightConfig.Id);
        }
        // 应用 world.yaml 中的 enabled 配置（默认启用）
        light->SetEnabled(lightConfig.Enabled);
        light->Direction = lightConfig.Direction;
        light->Color = lightConfig.Color;
        light->AmbientColor = lightConfig.AmbientColor;
        light->DiffuseColor = lightConfig.DiffuseColor;
        light->SpecularColor = lightConfig.SpecularColor;
        light->AmbientIntensity = lightConfig.AmbientIntensity;
        light->DiffuseIntensity = lightConfig.DiffuseIntensity;
        light->SpecularIntensity = lightConfig.SpecularIntensity;
        m_lights.push_back(light);
        i++;
    }

    // 创建点光源
    for (const auto &lightConfig: gConfig->PointLights) {
        // 名称优先使用 world.yaml 中的 name，未配置时回退为自动生成的索引名
        std::string pointName = lightConfig.Name.empty() ? std::format("light-{}", i + 1) : lightConfig.Name;
        auto light = new PointLight(pointName);
        // 若配置了 id，则覆盖默认自动生成的 UUID 用于稳定标识
        if (!lightConfig.Id.empty()) {
            light->SetUUID(lightConfig.Id);
        }
        // 应用 world.yaml 中的 enabled 配置（默认启用）
        light->SetEnabled(lightConfig.Enabled);
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
        auto model = Model::CreatePointLightModelV2();
        model->SetPosition(light->Position);
        light->SetModel(model);

        // 光源模型的着色器由 CreatePointLightModelV2 内部创建，未注册任何所有权；
        // 这里统一登记到 m_techniques 以便释放，避免内存泄漏（多个 mesh 共享同一 effect，需去重）
        for (const auto &mesh: model->GetMeshes()) {
            auto effect = mesh->GetEffect();
            if (effect != nullptr &&
                std::find(m_techniques.begin(), m_techniques.end(), effect) == m_techniques.end()) {
                m_techniques.push_back(effect);
            }
        }

        //
        m_light_models[light->GetUUID()] = model;
        std::cout << "Setup light finish" << std::endl;
        i++;
    }

    // 创建聚光灯
    for (const auto &lightConfig: gConfig->SpotLights) {
        // 名称优先使用 world.yaml 中的 name，未配置时回退为自动生成的索引名
        std::string spotName = lightConfig.Name.empty() ? std::format("spot-light-{}", i + 1) : lightConfig.Name;
        auto light = new SpotLight(spotName);
        // 若配置了 id，则覆盖默认自动生成的 UUID 用于稳定标识
        if (!lightConfig.Id.empty()) {
            light->SetUUID(lightConfig.Id);
        }
        // 应用 world.yaml 中的 enabled 配置（默认启用）
        light->SetEnabled(lightConfig.Enabled);
        light->Position = lightConfig.Position;
        light->Direction = lightConfig.Direction;
        light->Color = lightConfig.Color;
        light->AmbientColor = lightConfig.AmbientColor;
        light->DiffuseColor = lightConfig.DiffuseColor;
        light->SpecularColor = lightConfig.SpecularColor;
        light->AmbientIntensity = lightConfig.AmbientIntensity;
        light->DiffuseIntensity = lightConfig.DiffuseIntensity;
        light->SpecularIntensity = lightConfig.SpecularIntensity;
        light->Attenuation.Constant = lightConfig.Attenuation.Constant;
        light->Attenuation.Linear = lightConfig.Attenuation.Linear;
        light->Attenuation.Exp = lightConfig.Attenuation.Exp;
        light->Cutoff = lightConfig.Cutoff;
        light->OuterCutoff = lightConfig.OuterCutoff;
        m_lights.push_back(light);

        // 为聚光灯创建光源模型（复用点光源模型，仅用于可视化位置）
        auto model = Model::CreatePointLightModelV2();
        model->SetPosition(light->Position);
        light->SetModel(model);

        for (const auto &mesh: model->GetMeshes()) {
            auto effect = mesh->GetEffect();
            if (effect != nullptr &&
                std::find(m_techniques.begin(), m_techniques.end(), effect) == m_techniques.end()) {
                m_techniques.push_back(effect);
            }
        }

        m_light_models[light->GetUUID()] = model;
        std::cout << "Setup spot light finish" << std::endl;
        i++;
    }
    std::cout << "Setup lights finish" << std::endl;

    // 地形着色器最初绑定灯光时 m_lights 尚未创建完成(位于地形创建之后)，
    // 这里在灯光全部创建完成后重新绑定，确保地形能正确接收光源
    terrainEffect->SetLights(m_lights);

    for (const auto &modelConfig: gConfig->Models) {
        std::cout << "model name : " << modelConfig.Name << std::endl;
        std::cout << "mesh name  : " << modelConfig.Mesh.Name << std::endl;
        std::cout << "mesh file  : " << modelConfig.Mesh.File << std::endl;
        if (modelConfig.Mesh.File.empty()) {
            continue;
        }
        auto model_obj = new Model(modelConfig.Name);
        model_obj->LoadModel(modelConfig.Mesh.File);

        // 材质改为从 MTL 文件加载（自定义 MtlParser 解析），不再内联在 yaml 中
        MtlParser mtlParser;
        auto *material = mtlParser.ParseSingle(modelConfig.Material.File, modelConfig.Material.Name);
        if (material == nullptr) {
            // MTL 解析失败或材质名未找到时，退回默认材质，避免后续空指针
            material = new Material();
            material->Name = modelConfig.Material.Name;
            material->AmbientColor = glm::vec3(0.2f);
            material->DiffuseColor = glm::vec3(0.8f);
            material->SpecularColor = glm::vec3(0.0f);
            material->Shininess = 0.0f;
        }
        std::cout << "material loaded from mtl: " << material->Name
                  << " (Ka " << material->AmbientColor.r << "," << material->AmbientColor.g << "," << material->AmbientColor.b
                  << " Kd " << material->DiffuseColor.r << "," << material->DiffuseColor.g << "," << material->DiffuseColor.b
                  << " Ks " << material->SpecularColor.r << "," << material->SpecularColor.g << "," << material->SpecularColor.b
                  << " Ns " << material->Shininess << ")" << std::endl;

        auto *effect = new TechniqueLight(
            "default",
            modelConfig.ShaderVertFile,
            modelConfig.ShaderFragFile
        );
        effect->SetMaterial(material);
        effect->SetLights(m_lights);
        for (auto m: model_obj->GetMeshes()) {
            m->SetEffect(effect);
        }
        // 模型材质与着色器交由渲染器统一管理释放
        m_materials.push_back(material);
        m_techniques.push_back(effect);

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

    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_view_matrix = m_camera->GetViewMatrix();
    m_eye_pos = m_camera->GetEyePosition();

    glDepthMask(GL_FALSE);
    m_sky_dome->Draw(elapsed, m_projection_matrix, m_view_matrix, m_eye_pos);
    glDepthMask(GL_TRUE);

    // 坐标轴 gizmo 不在 3D 场景中绘制：由 mainwindow 在 ImGui 绘制阶段
    // 以屏幕空间叠加方式渲染于视口角落（见 mainwindow.cpp 的 RenderFrame）
    // 绘制地形
    m_terrain_manager->Draw(elapsed, m_projection_matrix, m_view_matrix, m_eye_pos, m_lights);

    // 绘制配置的粒子系统
    for (auto ps: m_particle_systems) {
        ps->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }

    // 绘制光源模型
    // 注意：光源模型与 light 的 m_model 指向同一对象，这里统一在绘制时同步位置与颜色，
    // 避免重复绘制、重复绑定着色器，以及 map 顺序与 lights 顺序不一致导致的颜色错乱
    for (const auto &kv: m_light_models) {
        auto lightModel = kv.second;
        if (lightModel == nullptr) {
            continue;
        }
        // 根据 UUID 查找对应光源，同步位置与颜色
        auto it = std::find_if(m_lights.begin(), m_lights.end(),
                               [&](Light *l) { return l->GetUUID() == kv.first; });
        if (it != m_lights.end()) {
            auto point_light = static_cast<PointLight *>(*it);
            lightModel->SetTranslate(point_light->Position);
            for (const auto &mesh: lightModel->GetMeshes()) {
                auto tech = mesh->GetEffect();
                tech->Enable();
                tech->SetUniform("color", point_light->Color);
            }
        }
        lightModel->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }

    // 绘制模型
    for (const auto &m: m_models) {
        m->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }
}

void Renderer::resize(int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, w, h);
    calculateProjectMatrix(w, h);
}

void Renderer::update(long long elapsed) {
    m_eye_pos = m_camera->GetEyePosition();
    
    // 地形为静态网格平面（无 LOD/无动态 chunk），无需每帧更新
    
    // 更新配置的粒子系统
    for (auto ps: m_particle_systems) {
        ps->Update(elapsed / 1000.0f);
    }
}

Model *Renderer::GetModel(const string &name) {
    for (auto model: m_models) {
        if (model->GetName() == name) {
            return model;
        }
    }
    return nullptr;
}

Model *Renderer::GetModelByUUID(const string &uuid) {
    for (auto model: m_models) {
        if (model->GetUUID() == uuid) {
            return model;
        }
    }
    return nullptr;
}

Light *Renderer::GetLightByUUID(const std::string &uuid) const {
    for (auto const light: m_lights) {
        if (light->GetUUID() == uuid) {
            return light;
        }
    }
    return nullptr;
}

float Renderer::GetFPS() const {
    return m_fps_counter->GetFPS();
}

void Renderer::SwitchCamera(int index) {
    if (index < 0 || static_cast<size_t>(index) >= m_cameras.size()) {
        std::cerr << "Invalid camera index: " << index << std::endl;
        return;
    }
    m_camera = m_cameras[index];

    // 通过设置位置来触发相机向量更新
    m_camera->m_front = glm::normalize(m_camera->m_target - m_camera->m_position);
    m_camera->m_right = glm::normalize(glm::cross(m_camera->m_front, m_camera->m_world_up));
    m_camera->m_up = glm::normalize(glm::cross(m_camera->m_right, m_camera->m_front));

    std::cout << "Camera switch to " << m_camera->GetName() << std::endl;
}

void Renderer::SerProjectionType(ProjectionType type) {
    m_projectionType = type;
}

const ProjectionType Renderer::GetProjectionType() const {
    return m_projectionType;
}

void Renderer::calculateProjectMatrix(const int w, const int h) {
    if (m_projectionType == ProjectionType::Perspective) {
        const float fov = gConfig->Clip.ClipFov; // 视野角度
        const float aspectRatio = (float) w / (float) (1 * h); // 宽高比
        const float nearPlane = gConfig->Clip.ClipNear; // 近平面距离
        const float farPlane = gConfig->Clip.ClipFar; // 远平面距离
        m_projection_matrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane); // 透视
    } else {
        // 设置正交投影
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

        // 定义一个合适的范围，这里我们假设使用 -10 到 10 的范围作为示例
        // 你可以根据实际需求调整这些值
        const float left = -20.0f * aspectRatio;
        float right = 20.0f * aspectRatio;
        float bottom = -20.0f;
        float top = 20.0f;

        float nearPlane = gConfig->Clip.ClipNear; // 近平面距离
        float farPlane = gConfig->Clip.ClipFar; // 远平面距离
        m_projection_matrix = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    }
}
