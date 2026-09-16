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
#include "technique/technique_terrain.h"
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
#include "camera/manipulator.h"
#include "camera/orbit_manipulator.h"
#include "fps/fps.h"
#include "shadow/shadow_framebuffer.h"
#include "postprocess/scene_framebuffer.h"
#include "debug/debug_draw.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <cmath>

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

    // 释放光源调试可视化系统（DebugDraw 自管 VAO/VBO，着色器归 m_techniques）
    if (m_debug_draw != nullptr) {
        delete m_debug_draw;
        m_debug_draw = nullptr;
    }

    // 释放所有摄像机
    // 注意：m_camera 始终是 m_cameras 中的一个元素，这里统一删除一次即可，
    // 不能单独再删 m_camera，否则会与这里的删除发生双重释放
    for (auto cam: m_cameras) {
        delete cam;
    }
    m_cameras.clear();

    // 释放相机操控器（不持有相机，仅绑定，相机生命期由上面 m_cameras 统一管理）
    delete m_manipulator;
    m_manipulator = nullptr;

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

    // 释放阴影深度贴图 FBO（深度 Pass 着色器 m_shadow_depth_tech 已随 m_techniques 释放）
    if (m_shadow_fbo != nullptr) {
        delete m_shadow_fbo;
        m_shadow_fbo = nullptr;
    }

    // 释放 HDR 场景帧缓冲（后处理着色器 m_post_tech 已随 m_techniques 释放）
    if (m_scene_fbo != nullptr) {
        delete m_scene_fbo;
        m_scene_fbo = nullptr;
    }
    // 释放后处理全屏三角形空 VAO
    if (m_post_vao != 0) {
        glDeleteVertexArrays(1, &m_post_vao);
        m_post_vao = 0;
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
    // 统一设置线宽，供线段/点光源模型等以线框方式绘制的对象使用(原实现在每次 Model::Draw 中重复设置)。
    // 【macOS 兼容】Metal 后端 GL_ALIASED_LINE_WIDTH_RANGE 只有 [1,1]，
    // 直接 glLineWidth(5) 会产生 GL_INVALID_VALUE；需先查询最大值再取 min。
    GLfloat lineWidthRange[2];
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
    glLineWidth(lineWidthRange[1] < 5.0f ? lineWidthRange[1] : 5.0f);
    // 设置 OpenGL 只绘制正面 , 不绘制背面
    // glEnable(GL_CULL_FACE);
    // 设置顺时针方向 CW : Clock Wind 顺时针方向, 默认是 GL_CCW : Counter Clock Wind 逆时针方向
    // glFrontFace(GL_CW);

    width = w;
    height = h;

    // 视野角度以配置为初始值，运行时可经 SetFov 调整（调试面板滑块）
    m_fov = gConfig->Clip.ClipFov;

    m_fps_counter = new FPSCounter();

    m_projection_matrix = glm::mat4(1.0f);
    m_view_matrix = glm::mat4(1.0f); //  默认生成的是一个单位矩阵（对角线上的元素为1）
    m_model_matrix = glm::mat4(1.0f); // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！
    m_eye_pos = glm::vec3(0, 0, 0);
    calculateProjectMatrix(w, h);

    // 创建屏幕空间坐标轴 gizmo（视口角落叠加的 ImGui 绘制，非 3D 网格）
    // 具体绘制见 mainwindow.cpp RenderFrame 中的 ApplyViewportAxisGizmo 调用
    m_axis = new Axis();

    // Create Plane：使用标准管线 ① 纯顶点颜色 shader（unlit，无光照）
    vector<Mesh *> plane_mesh = Mesh::CreatePlaneMesh();
    auto *plane_effect = new Technique("plane",
                                       "./resource/shader/unlit.vert",
                                       "./resource/shader/unlit.frag");

    auto *plane = new Model("plane");
    plane->SetScale(glm::vec3(5.0f, 5.0f, 5.0f));
    plane->SetMeshes(plane_mesh);
    for (auto m: plane_mesh) {
        m->SetEffect(plane_effect);
    }
    // 平面着色器交由渲染器统一管理释放
    m_techniques.push_back(plane_effect);

    // 平面为纯顶点颜色辅助网格（无材质），初始风格登记为 Unlit，供属性面板回显
    m_model_styles[plane] = RenderStyle::Unlit;

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
    auto *terrainEffect = new TechniqueTerrain("terrain",
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

    // ---- 初始化方向光阴影映射资源 ----
    // 阴影贴图分辨率 2048×2048：越高越清晰但越耗显存/带宽
    m_shadow_fbo = new ShadowFramebuffer();
    m_shadow_fbo->Init(2048, 2048);
    // 深度 Pass 专用着色器，注册进 m_techniques 以便与其它技术统一释放
    m_shadow_depth_tech = new Technique("shadow_depth",
                                        "./resource/shader/depth.vert",
                                        "./resource/shader/depth.frag");
    m_techniques.push_back(m_shadow_depth_tech);

    // ---- 初始化 HDR 场景帧缓冲与后处理 Pass（多 Pass 渲染框架）----
    // 场景 Pass 画到 RGBA16F FBO，后处理全屏 Pass 采样它做 tone mapping 再画到默认缓冲
    m_scene_fbo = new SceneFramebuffer();
    m_scene_fbo->Init(width, height);
    // 后处理着色器注册进 m_techniques 统一释放
    m_post_tech = new Technique("post",
                                "./resource/shader/post.vert",
                                "./resource/shader/post.frag");
    m_techniques.push_back(m_post_tech);
    // 空 VAO：全屏三角形坐标由顶点着色器 gl_VertexID 生成，无需顶点属性缓冲，
    // 但 Core Profile 在未绑定 VAO 时绘制会报错，故创建空 VAO 占位
    glGenVertexArrays(1, &m_post_vao);

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

    // 创建轨道相机操控器并绑定当前相机：所有相机交互（轨道/平移/缩放）
    // 由操控器承载，相机只做状态存储。初始轨道参数由相机当前姿态反推。
    m_manipulator = new OrbitManipulator();
    m_manipulator->SetCamera(m_camera);
    m_manipulator->ResetFromCamera();

    // 光源调试可视化系统（DebugDraw，方案 B）：独立于 Model 体系，
    // 每帧从 Light 对象直接收集线段顶点并批量绘制。
    // 调试着色器（debug.vert/debug.frag，纯顶点色忽略光照）由 Renderer 创建
    // 并共享给 DebugDraw，注册进 m_techniques 统一释放。
    auto *debugEffect = new Technique(
        "debug",
        "./resource/shader/debug.vert",
        "./resource/shader/debug.frag"
    );
    m_techniques.push_back(debugEffect);

    m_debug_draw = new DebugDraw();
    m_debug_draw->Init(debugEffect);

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

        // 登记模型 → 材质映射：模型切换到共享风格技术(Lit/Toon)时，
        // 共享技术的 GetMaterial() 会被其它模型覆盖，属性面板必须按模型查自己的材质
        m_model_materials[model_obj] = material;
        // 记录模型初始渲染风格：按 world.yaml 指定的片元着色器文件名推断，
        // 供属性面板下拉框回显当前风格（与 GetModelStyle 的默认值 Lit 区分）
        RenderStyle initialStyle = RenderStyle::Lit;
        const std::string &fragFile = modelConfig.ShaderFragFile;
        if (fragFile.find("toon") != std::string::npos) {
            initialStyle = RenderStyle::Toon;
        } else if (fragFile.find("textured") != std::string::npos) {
            initialStyle = RenderStyle::Textured;
        } else if (fragFile.find("unlit") != std::string::npos) {
            initialStyle = RenderStyle::Unlit;
        }
        m_model_styles[model_obj] = initialStyle;

        model_obj->SetScale(modelConfig.Scale);
        model_obj->SetTranslate(modelConfig.Position);
        model_obj->SetRotate(modelConfig.Rotation);

        m_models.push_back(model_obj);
    }

    // ---- 渲染风格技术池（运行时切换，基础版：无参数调节）----
    // 为四套标准着色器（unlit/textured/lit/toon）各创建一个共享 Technique，
    // 运行时 SetModelStyle 遍历模型 mesh 换用对应技术，实现界面切换渲染风格。
    // 共享实例登记进 m_techniques 统一释放；材质按模型单独登记（见 m_model_materials）。
    auto *styleUnlit = new Technique("style_unlit",
                                     "./resource/shader/unlit.vert",
                                     "./resource/shader/unlit.frag");
    m_style_techniques[RenderStyle::Unlit] = styleUnlit;
    m_techniques.push_back(styleUnlit);

    auto *styleTextured = new Technique("style_textured",
                                        "./resource/shader/textured.vert",
                                        "./resource/shader/textured.frag");
    m_style_techniques[RenderStyle::Textured] = styleTextured;
    m_techniques.push_back(styleTextured);

    // lit/toon 需要接收材质与灯光：使用 TechniqueLight（支持 SetMaterial/SetLights），
    // 与 unlit/textured 的普通 Technique 区分开
    auto *styleLit = new TechniqueLight("style_lit",
                                        "./resource/shader/lit.vert",
                                        "./resource/shader/lit.frag");
    m_style_techniques[RenderStyle::Lit] = styleLit;
    m_techniques.push_back(styleLit);

    auto *styleToon = new TechniqueLight("style_toon",
                                         "./resource/shader/toon.vert",
                                         "./resource/shader/toon.frag");
    m_style_techniques[RenderStyle::Toon] = styleToon;
    m_techniques.push_back(styleToon);

    std::cout << "Setup world finish" << std::endl;
}

void Renderer::draw(long long elapsed) {
    // 计算投影竖切像机矩阵
    // 绘制帧数量加1
    m_fps_counter->Add();

    // 视口背景色：默认深灰，调试面板 ColorEdit3 可实时修改（见 SetClearColor）
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_view_matrix = m_camera->GetViewMatrix();
    m_eye_pos = m_camera->GetPosition();

    // ---- 方向光阴影深度 Pass ----
    // 先用光源视角把场景写进深度贴图，主渲染 Pass 再采样它判定阴影。
    // 仅在启用阴影且存在已启用的方向光时执行；否则本帧不启用阴影采样。
    m_shadow_map_ready = false;
    // 每帧先重置光源摄像机参数的有效标志：仅当本帧存在已启用的方向光时才置为有效
    m_shadow_camera.available = false;
    if (m_shadows_enabled && m_shadow_fbo != nullptr && m_shadow_depth_tech != nullptr) {
        DirectionLight *dirLight = nullptr;
        for (auto light: m_lights) {
            if (light->GetLightType() == LightTypeDirection && light->IsEnabled()) {
                dirLight = static_cast<DirectionLight *>(light);
                break;
            }
        }

        if (dirLight != nullptr) {
            // 保存当前主渲染视口（含 Dock 中央面板的偏移 origin），
            // 阴影 Pass 会改写到 2048×2048，结束后必须原样恢复，否则场景会从(0,0)绘制导致画面偏移/被裁剪
            GLint prevMainViewport[4];
            glGetIntegerv(GL_VIEWPORT, prevMainViewport);

            // 计算光源空间矩阵：平行光用正交投影（覆盖场景范围），保证阴影精度与覆盖平衡。
            // 光照计算用 toLight = -Direction（光源在 -Direction 方向远处，即场景上方），
            // 因此阴影深度 Pass 的光源"摄像机"也必须放在 -Direction 一侧并朝场景看，
            // 否则深度贴图从相反方向生成，深度比较永远对不齐光照，阴影无法显示。
            glm::vec3 lightDir = glm::normalize(dirLight->Direction);
            // 光源摄像机参数统一存入 m_shadow_camera（供 ImGui 面板展示与矩阵计算共用同一来源），
            // 后续 lightProjection/lightView 矩阵全部由这些字段构建，避免魔法数在计算处重复出现。
            // 正交投影范围：±210 单位，覆盖全部地形（地形顶点范围 ±100 × 模型缩放 2.1 = ±210）。
            // sphere 直径约 3 单位，在 420 单位宽的投影中仅占 ~0.7%，阴影会明显像素化，
            // 但这是覆盖全部地形的代价。若需高质量 sphere 阴影，应使用 Cascaded Shadow Maps。
            // 近/远平面：光源距场景约 30 单位，sphere 在 y=10（距光源 20 单位），
            // 地形在 y=0（距光源 30 单位），故 near=1、far=100 足够覆盖。
            // orthoLeft/Right/Bottom/Top、nearPlane/farPlane 使用 ShadowCameraParams 的默认值
            // （±210 / 1 / 100），无需每帧重新赋值
            m_shadow_camera.direction = lightDir;
            m_shadow_camera.position = -lightDir * 30.0f; // 光源位于场景上方（-Direction 远处），向下照射
            m_shadow_camera.lookAt = glm::vec3(0.0f);

            // 当光源恰好垂直位于场景正上方（如太阳方向 (0,-1,0) 纯直下）时，
            // lookAt 的 look 向量与默认 up=(0,1,0) 完全平行，cross 得零向量，
            // normalize 产生 NaN 导致所有顶点深度无效、深度贴图为空。
            // 此时改用 Z 轴（0,0,-1）作 up，避免退化。
            // 【说明】若用 X 轴(1,0,0) 作 up，光源视图坐标系会发生 90° 旋转：
            //   世界 z → 光源 x、世界 x → 光源 y，导致阴影贴图采样坐标轴互换，
            //   表现为纯垂直光下球体出现"沿 z=0 经线"的红/黑异常分界（用户报告的问题）。
            // 改用 Z 轴后，光源视图 x 轴 = 世界 x、y 轴 = 世界 z，深度仍沿 -y，
            // 坐标轴不再异常互换，阴影分界恢复为正确的水平纬线（顶部亮/底部暗）。
            glm::vec3 lookDir = glm::normalize(m_shadow_camera.lookAt - m_shadow_camera.position);
            glm::vec3 safeUp = (fabs(lookDir.y) > 0.999f)
                                 ? glm::vec3(0.0f, 0.0f, 1.0f)
                                 : glm::vec3(0.0f, 1.0f, 0.0f);
            m_shadow_camera.up = safeUp;

            // 视图/投影矩阵统一由 m_shadow_camera 字段构建（字段是唯一参数来源）
            m_shadow_camera.lightView = glm::lookAt(m_shadow_camera.position, m_shadow_camera.lookAt, m_shadow_camera.up);
            m_shadow_camera.lightProjection = glm::ortho(m_shadow_camera.orthoLeft, m_shadow_camera.orthoRight,
                                                         m_shadow_camera.orthoBottom, m_shadow_camera.orthoTop,
                                                         m_shadow_camera.nearPlane, m_shadow_camera.farPlane);
            m_shadow_camera.available = true;

            m_light_space = m_shadow_camera.lightProjection * m_shadow_camera.lightView;

            // 绑定阴影 FBO 并清空深度，随后以"只写深度"方式重画场景（地形 + 模型），
            // 生成光源视角的深度贴图供主 Pass 阴影判断使用
            m_shadow_fbo->BindForWrite();
            Mesh::SetShadowDepthState(m_shadow_depth_tech, m_light_space, true);

            // m_terrain_manager->Draw(elapsed, m_projection_matrix, m_view_matrix, m_eye_pos, m_lights);
            for (const auto &m: m_models) {
                m->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
            }

            Mesh::SetShadowDepthState(nullptr, glm::mat4(1.0f), false);
            m_shadow_fbo->Unbind();
            // 恢复之前保存的主渲染视口（阴影 Pass 改写过视口，不解绑/不恢复会残余错误大小与偏移）
            glViewport(prevMainViewport[0], prevMainViewport[1], prevMainViewport[2], prevMainViewport[3]);


            // 把阴影深度贴图绑定到纹理单元2（单元0/1 已被漫反射/法线贴图占用），
            // 后续每个 Mesh::Draw 会通过 SetShadowMap(2) 通知着色器采样它
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_shadow_fbo->GetDepthTexture());
            // 标记本帧已有阴影贴图，主 Pass 的 Mesh::Draw 才会启用阴影采样
            m_shadow_map_ready = true;


            // 把本帧阴影状态同步给地形管理器，最终存入 TechniqueTerrain，
            // 主 Pass 绘制地形时由其自管阴影采样（见 TerrainChunk::SetShadowState / ApplyShadowState）
            m_terrain_manager->SetShadowState(m_shadow_map_ready,
                                              m_shadow_fbo->GetDepthTexture(),
                                              m_light_space);
        }
    }
    // 同步阴影贴图可用状态到 Mesh（供 Mesh::Draw 决定是否上传 lightSpace / 绑定 shadowMap）
    Mesh::SetShadowMapAvailable(m_shadow_map_ready);

    // ---- 场景 Pass：所有 3D 内容渲染到 HDR 场景 FBO ----
    // 原实现直接画进默认帧缓冲；现在先画到 RGBA16F 离屏 FBO（保留 HDR 精度），
    // 结束后由后处理 Pass 统一做 tone mapping 输出到默认缓冲
    m_scene_fbo->BindForWrite();

    glDepthMask(GL_FALSE);
    m_sky_dome->Draw(elapsed, m_projection_matrix, m_view_matrix, m_eye_pos);
    glDepthMask(GL_TRUE);

    // 坐标轴 gizmo 不在 3D 场景中绘制：由 mainwindow 在 ImGui 绘制阶段
    // 以屏幕空间叠加方式渲染于视口角落（见 mainwindow.cpp 的 RenderFrame）

    // ---- 线框模式 ----
    // 开启时地形与模型以线框渲染（便于观察布线），天空穹/粒子保持原样：
    // 天空穹始终 FILL（半球线框会视觉污染背景），粒子是点图元不受 polygonMode 影响。
    // 阴影 Pass 在线框开关之前已结束，深度贴图始终以实体生成，不受影响。
    // 修改全局 GL 状态必须保存并在作用域结束恢复（AGENTS.md 教训2）。
    // 【macOS 兼容】开关与恢复都必须整体用 GL_FRONT_AND_BACK 调用：
    //   Metal 后端（OpenGL 4.1 Metal）对 glPolygonMode(GL_FRONT/_BACK, ...) 单独设置
    //   face 的调用返回 GL_INVALID_ENUM 且状态不变。若恢复时分开设置 front/back，
    //   线框关闭后 GL_LINE 残留，连后处理全屏三角形也会退化为边线导致画面全黑。
    //   front/back 光栅化模式始终一致（本项目从未单面设置），故可直接取 front 值整体恢复。
    GLint prevPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, prevPolygonMode);
    if (m_wireframeEnabled) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // 绘制地形
    m_terrain_manager->Draw(elapsed, m_projection_matrix, m_view_matrix, m_eye_pos, m_lights);

    // 网格地面辅助线：XZ 平面世界网格，独立于光源 gizmo 开关（见 DrawGrid）
    if (m_gridEnabled && m_debug_draw != nullptr) {
        DrawGrid();
        // 线框模式下不额外恢复状态：DebugDraw 是 GL_LINES，polygonMode 不影响线段图元
        m_debug_draw->Render(m_projection_matrix, m_view_matrix);
    }

    // 绘制配置的粒子系统
    for (auto ps: m_particle_systems) {
        ps->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }

    // 光源调试可视化（DebugDraw）：直接遍历灯光列表，把 gizmo 顶点收集进
    // DebugDraw 后统一渲染。位置/朝向/衰减每帧从 Light 对象读取，
    // 编辑器修改后自动跟随，无"先创建后绑定"的同步问题。
    // 开关关闭时整段跳过，不收集也不提交（ImGui 阴影面板可配置）。
    if (m_debugDrawEnabled) {
        for (auto light: m_lights) {
            if (light == nullptr) {
                continue;
            }
            switch (light->GetLightType()) {
                case LightTypeSpot:
                    m_debug_draw->DrawSpotLight(static_cast<SpotLight *>(light));
                    break;
                case LightTypeDirection:
                    m_debug_draw->DrawDirectionLight(static_cast<DirectionLight *>(light));
                    break;
                case LightTypePoint:
                    m_debug_draw->DrawPointLight(static_cast<PointLight *>(light));
                    break;
                default:
                    break;
            }
        }
        // 光照范围可视化：在 gizmo 基础上叠加更大的影响范围线框，
        // 点光源为球形线框、聚光灯为锥体线框、方向光无影响范围概念跳过。
        if (m_lightRangeEnabled) {
            for (auto light: m_lights) {
                if (light == nullptr) {
                    continue;
                }
                switch (light->GetLightType()) {
                    case LightTypePoint: {
                        auto *pointLight = static_cast<PointLight *>(light);
                        const float radius = DebugDraw::ComputePointLightRadius(pointLight);
                        m_debug_draw->DrawSphereWireframe(pointLight->Position, radius,
                                                          glm::vec3(0.3f, 1.0f, 0.6f), 24);
                        break;
                    }
                    case LightTypeSpot: {
                        auto *spotLight = static_cast<SpotLight *>(light);
                        // 用外锥角（OuterCutoff）绘制完整半影锥体范围
                        m_debug_draw->DrawConeWireframe(spotLight->Position, spotLight->Direction,
                                                        spotLight->OuterCutoff, 30.0f,
                                                        glm::vec3(1.0f, 0.75f, 0.2f), 24);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        // 批量提交：上传本帧全部 gizmo 线段，一次 glDrawArrays(GL_LINES) 绘制（内部自动 Clear）
        m_debug_draw->Render(m_projection_matrix, m_view_matrix);
    }

    // 绘制模型
    for (const auto &m: m_models) {
        // 共享风格技术（lit/toon）的材质 uniform 会被绘制顺序在后的同技术模型覆盖：
        // 每帧按模型重设材质，保证材质编辑与多模型共用风格技术时互不串色
        if (!m->GetMeshes().empty()) {
            auto matIt = m_model_materials.find(m);
            Technique *effect = m->GetMeshes()[0]->GetEffect();
            if (matIt != m_model_materials.end() && effect != nullptr && matIt->second != nullptr) {
                effect->SetMaterial(matIt->second);
            }
        }
        m->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }

    // 法线可视化：在模型绘制之后收集所有模型顶点的法线线段，
    // 颜色按法线方向编码，便于检查法线朝向是否正确
    if (m_normalVisualizationEnabled && m_debug_draw != nullptr) {
        CollectModelNormals();
        m_debug_draw->Render(m_projection_matrix, m_view_matrix);
    }

    // 鼠标拾取高亮：把命中的世界空间 AABB 画成线框盒叠加在场景之上
    // （DebugDraw 支持 GL_LINES，不受上方线框/FILL 多边形模式开关影响）；
    // 空盒（min==max）代表无高亮，直接跳过
    if (m_debug_draw != nullptr && m_pickHighlightMin != m_pickHighlightMax) {
        m_debug_draw->DrawBoxWireframe(m_pickHighlightMin, m_pickHighlightMax,
                                       glm::vec3(1.0f, 0.85f, 0.2f));
        m_debug_draw->Render(m_projection_matrix, m_view_matrix);
    }

    // 线框作用域结束：恢复进入场景 Pass 前的多边形光栅化模式（GL_FILL）。
    // 必须整体用 GL_FRONT_AND_BACK 恢复（macOS Metal 兼容性，见上方线框模式注释）
    if (m_wireframeEnabled) {
        glPolygonMode(GL_FRONT_AND_BACK, prevPolygonMode[0]);
    }

    // 场景 Pass 结束，回到默认帧缓冲（SceneFramebuffer 内部恢复主视口）
    m_scene_fbo->Unbind();

    // ---- 后处理 Pass：全屏三角形采样 HDR 场景纹理，做 tone mapping 后画到默认缓冲 ----
    // 后处理是 2D 全屏操作，不需要深度测试与混合；
    // 先保存再禁用、绘制后恢复，遵循 AGENTS.md "修改全局 OpenGL 状态必须保存和恢复"的教训
    const GLboolean depthTestWas = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWas = glIsEnabled(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_post_tech->Enable();
    m_post_tech->SetUniform("sceneTex", 0);   // 场景 HDR 纹理绑定到纹理单元0
    m_post_tech->SetUniform("toneMapMode", m_toneMappingEnabled ? 1 : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_scene_fbo->GetColorTexture());

    glBindVertexArray(m_post_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);   // 全屏三角形：3 个顶点覆盖整个视口
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (depthTestWas) {
        glEnable(GL_DEPTH_TEST);
    }
    if (blendWas) {
        glEnable(GL_BLEND);
    }
}

void Renderer::resize(int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, w, h);
    calculateProjectMatrix(w, h);

    // 场景 FBO 与视口同尺寸；尺寸变化时重建（SceneFramebuffer::Init 同尺寸幂等跳过）
    if (m_scene_fbo != nullptr) {
        m_scene_fbo->Init(w, h);
    }
}

void Renderer::update(long long elapsed) {
    m_eye_pos = m_camera->GetPosition();
    
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

/*
 * 运行时切换模型渲染风格（基础版，无参数调节）
 *
 * 思路（业界常见"共享技术池"做法）：四套标准着色器各持一个共享 Technique
 * （见 init 中 m_style_techniques），切换 = 把模型所有 mesh 的 effect 换成池中实例。
 * Mesh::Draw 每帧会重设变换矩阵/相机/灯光/阴影等 uniform，故切换后下一帧自动生效。
 * 材质是唯一的"隐性状态"，需在切换时立即随模型重设（见 m_model_materials），
 * 避免共享技术之间残留上一模型的材质 uniform。
 */
void Renderer::SetModelStyle(Model *model, RenderStyle style) {
    if (model == nullptr) {
        return;
    }
    auto it = m_style_techniques.find(style);
    if (it == m_style_techniques.end()) {
        return;
    }

    // 切换技术
    for (auto *mesh: model->GetMeshes()) {
        if (mesh != nullptr) {
            mesh->SetEffect(it->second);
        }
    }
    m_model_styles[model] = style;

    // 同步材质：lit/toon 技术依赖材质 uniform，用本模型材质立即重设，
    // 防止共享技术残留其它模型（或此前的默认材质）的 uniform 值
    auto matIt = m_model_materials.find(model);
    if (matIt != m_model_materials.end() && matIt->second != nullptr) {
        it->second->SetMaterial(matIt->second);
    }
}

RenderStyle Renderer::GetModelStyle(Model *model) const {
    auto it = m_model_styles.find(model);
    if (it != m_model_styles.end()) {
        return it->second;
    }
    return RenderStyle::Lit; // 默认与 old.world.yaml 传统光照一致
}

Material *Renderer::GetModelMaterial(Model *model) const {
    auto it = m_model_materials.find(model);
    if (it != m_model_materials.end()) {
        return it->second;
    }
    return nullptr;
}

/* 设置鼠标拾取结果的线框高亮盒（世界空间 AABB），draw 末尾叠加绘制 */
void Renderer::SetPickHighlight(const glm::vec3 &min, const glm::vec3 &max) {
    m_pickHighlightMin = min;
    m_pickHighlightMax = max;
}

/* 清除拾取高亮：置为空盒（min==max），draw 末尾检测空盒跳过绘制 */
void Renderer::ClearPickHighlight() {
    m_pickHighlightMin = glm::vec3(0.0f);
    m_pickHighlightMax = glm::vec3(0.0f);
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

    // 操控器重新绑定到新相机，并由新相机姿态反推轨道参数：
    // 每次切换后轨道中心落在新相机正前方（保持当前缩放级别），视角不跳变
    m_manipulator->SetCamera(m_camera);
    m_manipulator->ResetFromCamera();

    std::cout << "Camera switch to " << m_camera->GetName() << std::endl;
}

void Renderer::SerProjectionType(ProjectionType type) {
    m_projectionType = type;
}

const ProjectionType Renderer::GetProjectionType() const {
    return m_projectionType;
}

unsigned int Renderer::GetShadowDepthTexture() const {
    // 阴影 FBO 未创建（阴影禁用或初始化前）时返回 0，
    // 供 ImGui 调试面板判断当前是否可显示深度贴图
    return (m_shadow_fbo != nullptr) ? m_shadow_fbo->GetDepthTexture() : 0;
}

bool Renderer::IsShadowMapReady() const {
    // 本帧阴影深度 Pass 是否已执行并生成有效深度贴图
    return m_shadow_map_ready;
}

void Renderer::SetFov(float fov) {
    // 范围钳制：避免极端值导致投影矩阵数值异常
    m_fov = glm::clamp(fov, 10.0f, 160.0f);
    // 视野变化需立即生效：以当前窗口尺寸重算投影矩阵
    calculateProjectMatrix(width, height);
}

/*
 * 收集世界网格辅助线（XZ 平面）
 *
 * 地形范围 ±100 × 模型缩放 2.1 = ±210，故网格覆盖 ±200；
 * 间距 20 单位，共 21+21 条等距线段，肉眼可辨且开销极小。
 * 每条线仅两个调试顶点，通过 DebugDraw::DrawLine 收集后统一提交。
 * 网格仅与开关（m_gridEnabled）绑定，不受光源 gizmo 开关（m_debugDrawEnabled）影响。
 */
void Renderer::DrawGrid() {
    constexpr float kExtent = 200.0f; // 覆盖范围（半边长，略小于地形最远顶点 ±210）
    constexpr float kStep = 20.0f;    // 网格间距（单位）
    constexpr float kY = 0.02f;       // 略高于地面（y=0），避免与地形共面 Z-fighting
    const glm::vec3 gridColor(0.35f, 0.35f, 0.35f); // 常规网格线：深灰

    // 沿 X 与 Z 两个方向生成相互垂直的两组等距平行线
    for (float pos = -kExtent; pos <= kExtent + 0.5f; pos += kStep) {
        // X 方向线：固定 x=pos，从 z=-kExtent 到 z=+kExtent
        m_debug_draw->DrawLine(glm::vec3(pos, kY, -kExtent), glm::vec3(pos, kY, kExtent), gridColor);
        // Z 方向线：固定 z=pos，从 x=-kExtent 到 x=+kExtent
        m_debug_draw->DrawLine(glm::vec3(-kExtent, kY, pos), glm::vec3(kExtent, kY, pos), gridColor);
    }
}

/*
 * 收集模型法线线段到 DebugDraw
 *
 * 遍历所有模型的每个网格的每个顶点，把顶点位置沿法线方向延伸一小段，
 * 作为线段加入 DebugDraw。顶点位置用模型矩阵变换到世界空间，
 * 法线用模型矩阵的逆转置矩阵（法线矩阵）变换，保证非均匀缩放下方向正确。
 *
 * 法线线段长度使用统一的世界空间固定值（m_normalLength，ImGui 可调），
 * 保证所有模型法线等长、视觉一致；不再按网格包围盒比例计算，
 * 避免大模型法线过长、小模型法线过短的尺度差异。
 */
void Renderer::CollectModelNormals() {
    for (const auto &model: m_models) {
        if (model == nullptr) {
            continue;
        }
        // 构造与 Model::Draw 相同的模型矩阵：translate * scale * rotate(Y)
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, model->GetPosition());
        modelMat = glm::scale(modelMat, model->GetScale());
        modelMat = glm::rotate(modelMat, glm::radians(model->GetRotation()), glm::vec3(0.0f, 1.0f, 0.0f));

        // 法线矩阵 = 模型矩阵的逆转置，用于把模型空间的法线变换到世界空间，
        // 保证非均匀缩放时法线方向正确（scale=1 时等价于 modelMat 的旋转部分）
        glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

        const auto meshes = model->GetMeshes();
        for (auto *mesh: meshes) {
            if (mesh == nullptr) {
                continue;
            }
            for (const auto &vertex: mesh->GetVertices()) {
                glm::vec3 worldPos = glm::vec3(modelMat * glm::vec4(vertex.Position, 1.0f));
                glm::vec3 worldNormal = glm::normalize(normalMat * vertex.Normal);
                // 统一长度（世界空间固定值），所有模型法线视觉等长
                m_debug_draw->DrawNormal(worldPos, worldNormal, m_normalLength);
            }
        }
    }
}

void Renderer::calculateProjectMatrix(const int w, const int h) {
    if (m_projectionType == ProjectionType::Perspective) {
        const float fov = m_fov; // 视野角度（member，调试面板可调）
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
