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
#include "render_context.h"
#include "utils/gl_state_guard.h"
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
#include "animation/animation.h"
#include "animation/anim_target.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <cmath>
#include <cstdlib>

Renderer::Renderer() : m_eye_pos(0) {
}

Renderer::~Renderer() {
    // 动态成员由 unique_ptr 自管，此处只释放渲染器直接持有的 OpenGL 资源。

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
    m_fov = gConfig->m_clip.m_clip_fov;

    m_fps_counter = std::make_unique<FPSCounter>();

    m_projection_matrix = glm::mat4(1.0f);
    m_view_matrix = glm::mat4(1.0f); //  默认生成的是一个单位矩阵（对角线上的元素为1）
    m_eye_pos = glm::vec3(0, 0, 0);
    calculateProjectMatrix(w, h);

    // 创建屏幕空间坐标轴 gizmo（视口角落叠加的 ImGui 绘制，非 3D 网格）
    // 具体绘制见 mainwindow.cpp RenderFrame 中的 ApplyViewportAxisGizmo 调用
    m_axis = std::make_unique<Axis>();

    // 创建地形管理器（固定尺寸网格平面，无 LOD）
    m_terrain_manager = std::make_unique<TerrainManager>();
    TerrainConfig terrainConfig;
    terrainConfig.m_plane_size = gConfig->m_terrain.m_plane_size;    // 平面尺寸 100×100 单位
    terrainConfig.m_resolution = gConfig->m_terrain.m_resolution;  // 网格分辨率
    terrainConfig.m_height_scale = gConfig->m_terrain.m_height_scale; // 最大高度（0 = 平坦）
    terrainConfig.m_noise_seed = gConfig->m_terrain.m_noise_seed;     // 噪声种子
    m_terrain_manager->Init(terrainConfig);

    // 灯光为 unique_ptr 容器，收集非拥有指针视图供 SetLights 等接口使用
    std::vector<Light *> lights_raw;

    // 创建地形着色器和材质
    auto *terrainEffect = new TechniqueTerrain("terrain",
                                               "./resource/shader/terrain.vert",
                                               "./resource/shader/terrain.frag");
    Material *terrainMaterial = new Material();
    terrainMaterial->m_ambient_color = glm::vec3(0.3f, 0.3f, 0.3f);
    terrainMaterial->m_diffuse_color = glm::vec3(0.8f, 0.8f, 0.8f);
    terrainMaterial->m_specular_color = glm::vec3(0.5f, 0.5f, 0.5f);
    terrainMaterial->m_shininess = 32.0f;
    terrainEffect->SetMaterial(terrainMaterial);
    terrainEffect->SetLights(lights_raw);
    m_terrain_manager->SetTechnique(terrainEffect);
    // 交由渲染器统一管理地形着色器与材质的释放
    m_techniques.push_back(std::unique_ptr<Technique>(terrainEffect));
    m_materials.push_back(std::unique_ptr<Material>(terrainMaterial));

    // 创建并绑定地形纹理
    m_terrain_texture = Utils::CreateCheckerboardTexture(512, 512, 64);
    m_terrain_manager->SetTexture(m_terrain_texture);

    // ---- 初始化方向光阴影映射资源 ----
    m_shadow_pass = std::make_unique<ShadowPass>();
    m_shadow_pass->Init();

    // ---- 初始化 HDR 场景帧缓冲与后处理 Pass（多 Pass 渲染框架）----
    // 场景 Pass 画到 RGBA16F FBO，后处理全屏 Pass 采样它做 tone mapping 再画到默认缓冲
    m_scene_fbo = std::make_unique<SceneFramebuffer>();
    m_scene_fbo->Init(width, height);
    m_post_process_pass = std::make_unique<PostProcessPass>();
    m_post_process_pass->Init();

    m_sky_dome = std::make_unique<SkyDome>();
    m_sky_dome->Init(
        gConfig->m_sky_dome.m_radius,
        gConfig->m_sky_dome.m_sectors,
        gConfig->m_sky_dome.m_stacks
    );
    m_sky_dome->SetHorizonColor(gConfig->m_sky_dome.m_horizon_color);
    m_sky_dome->SetZenithColor(gConfig->m_sky_dome.m_zenith_color);
    m_sky_dome->SetGroundColor(gConfig->m_sky_dome.m_ground_color);

    // Create Particle Systems from config
    for (const auto &particleConfig: gConfig->m_particles) {
        auto ps = std::make_unique<ParticleSystem>();
        ps->Init(particleConfig.m_position);
        
        // 应用配置
        auto *emitter = ps->GetEmitter();
        emitter->SetMaxParticles(particleConfig.m_max_particles);
        ps->ReallocateVBO();
        emitter->m_emit_rate = particleConfig.m_emit_rate;
        emitter->m_min_life = particleConfig.m_min_life;
        emitter->m_max_life = particleConfig.m_max_life;
        emitter->m_min_size = particleConfig.m_min_size;
        emitter->m_max_size = particleConfig.m_max_size;
        emitter->m_min_velocity = particleConfig.m_min_velocity;
        emitter->m_max_velocity = particleConfig.m_max_velocity;
        emitter->m_min_size_end = particleConfig.m_min_size_end;
        emitter->m_max_size_end = particleConfig.m_max_size_end;
        emitter->m_gravity = particleConfig.m_gravity;
        emitter->m_drag = particleConfig.m_drag;
        
        m_particle_systems.push_back(std::move(ps));
    }

    for (const auto &cameraConfig: gConfig->m_cameras) {
        auto camera = std::make_unique<Camera>(
            cameraConfig.m_position,
            cameraConfig.m_target,
            cameraConfig.m_up
        );
        camera->m_name = cameraConfig.m_name;
        camera->SetProjectionType(cameraConfig.m_projection);
        m_cameras.push_back(std::move(camera));
    }
    // 默认摄像机取配置里的第一台。m_camera 在 draw() 等处被无条件解引用，
    // 一台都没配就跑不起来，所以不做兜底，直接报错退出，别用隐式默认视角掩盖配置错误
    if (m_cameras.empty()) {
        std::cerr << "[error] world.yaml 未配置任何摄像机（world.cameras），无法确定默认摄像机" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // 复用 m_cameras 中的第一个摄像机作为当前摄像机，避免重复创建造成内存泄漏
    m_camera = m_cameras[0].get();
    // 相机就绪后用当前摄像机的投影属性校准投影矩阵（构造前期无相机时按默认透视计算过）
    calculateProjectMatrix(width, height);

    // 创建轨道相机操控器并绑定当前相机：所有相机交互（轨道/平移/缩放）
    // 由操控器承载，相机只做状态存储。初始轨道参数由相机当前姿态反推。
    m_manipulator = std::make_unique<OrbitManipulator>();
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
    m_techniques.push_back(std::unique_ptr<Technique>(debugEffect));

    m_debug_draw = std::make_unique<DebugDraw>();
    m_debug_draw->Init(debugEffect);

    int i = 0;
    // 创建方向光（平行光）
    for (const auto &lightConfig: gConfig->m_direction_lights) {
        // 名称优先使用 world.yaml 中的 name，未配置时回退为自动生成的索引名
        std::string dirName = lightConfig.m_name.empty() ? std::format("dir-light-{}", i + 1) : lightConfig.m_name;
        auto light = new DirectionLight(dirName);
        // 若配置了 id，则覆盖默认自动生成的 UUID 用于稳定标识
        if (!lightConfig.m_id.empty()) {
            light->SetUUID(lightConfig.m_id);
        }
        // 应用 world.yaml 中的 enabled 配置（默认启用）
        light->SetEnabled(lightConfig.m_enabled);
        light->m_direction = lightConfig.m_direction;
        light->m_color = lightConfig.m_color;
        light->m_ambient_color = lightConfig.m_ambient_color;
        light->m_diffuse_color = lightConfig.m_diffuse_color;
        light->m_specular_color = lightConfig.m_specular_color;
        light->m_ambient_intensity = lightConfig.m_ambient_intensity;
        light->m_diffuse_intensity = lightConfig.m_diffuse_intensity;
        light->m_specular_intensity = lightConfig.m_specular_intensity;
        m_lights.push_back(std::unique_ptr<Light>(light));
        lights_raw.push_back(light);

        i++;
    }

    // 创建点光源
    for (const auto &lightConfig: gConfig->m_point_lights) {
        // 名称优先使用 world.yaml 中的 name，未配置时回退为自动生成的索引名
        std::string pointName = lightConfig.m_name.empty() ? std::format("light-{}", i + 1) : lightConfig.m_name;
        auto light = new PointLight(pointName);
        // 若配置了 id，则覆盖默认自动生成的 UUID 用于稳定标识
        if (!lightConfig.m_id.empty()) {
            light->SetUUID(lightConfig.m_id);
        }
        // 应用 world.yaml 中的 enabled 配置（默认启用）
        light->SetEnabled(lightConfig.m_enabled);
        light->m_color = lightConfig.m_color;
        light->m_position = lightConfig.m_position;
        light->m_ambient_color = lightConfig.m_ambient_color;
        light->m_diffuse_color = lightConfig.m_diffuse_color;
        light->m_specular_color = lightConfig.m_specular_color;
        light->Attenuation.m_constant = lightConfig.Attenuation.m_constant;
        light->Attenuation.m_linear = lightConfig.Attenuation.m_linear;
        light->Attenuation.m_exp = lightConfig.Attenuation.m_exp;
        m_lights.push_back(std::unique_ptr<Light>(light));
        lights_raw.push_back(light);

        std::cout << "Setup light finish" << std::endl;
        i++;
    }

    // 创建聚光灯
    for (const auto &lightConfig: gConfig->m_spot_lights) {
        // 名称优先使用 world.yaml 中的 name，未配置时回退为自动生成的索引名
        std::string spotName = lightConfig.m_name.empty() ? std::format("spot-light-{}", i + 1) : lightConfig.m_name;
        auto light = new SpotLight(spotName);
        // 若配置了 id，则覆盖默认自动生成的 UUID 用于稳定标识
        if (!lightConfig.m_id.empty()) {
            light->SetUUID(lightConfig.m_id);
        }
        // 应用 world.yaml 中的 enabled 配置（默认启用）
        light->SetEnabled(lightConfig.m_enabled);
        light->m_position = lightConfig.m_position;
        light->m_direction = lightConfig.m_direction;
        light->m_color = lightConfig.m_color;
        light->m_ambient_color = lightConfig.m_ambient_color;
        light->m_diffuse_color = lightConfig.m_diffuse_color;
        light->m_specular_color = lightConfig.m_specular_color;
        light->m_ambient_intensity = lightConfig.m_ambient_intensity;
        light->m_diffuse_intensity = lightConfig.m_diffuse_intensity;
        light->m_specular_intensity = lightConfig.m_specular_intensity;
        light->Attenuation.m_constant = lightConfig.Attenuation.m_constant;
        light->Attenuation.m_linear = lightConfig.Attenuation.m_linear;
        light->Attenuation.m_exp = lightConfig.Attenuation.m_exp;
        light->m_cutoff = lightConfig.m_cutoff;
        light->m_outer_cutoff = lightConfig.m_outer_cutoff;
        m_lights.push_back(std::unique_ptr<Light>(light));
        lights_raw.push_back(light);

        std::cout << "Setup spot light finish" << std::endl;
        i++;
    }
    std::cout << "Setup lights finish" << std::endl;

    // 地形着色器最初绑定灯光时 m_lights 尚未创建完成(位于地形创建之后)，
    // 这里在灯光全部创建完成后重新绑定，确保地形能正确接收光源
    terrainEffect->SetLights(lights_raw);

    for (const auto &modelConfig: gConfig->m_models) {
        std::cout << "model name : " << modelConfig.m_name << std::endl;
        std::cout << "mesh name  : " << modelConfig.m_mesh.m_name << std::endl;
        std::cout << "mesh file  : " << modelConfig.m_mesh.m_file << std::endl;
        if (modelConfig.m_mesh.m_file.empty()) {
            continue;
        }
        auto model_obj = new Model(modelConfig.m_name);
        model_obj->LoadModel(modelConfig.m_mesh.m_file);

        // 材质改为从 MTL 文件加载（自定义 MtlParser 解析），不再内联在 yaml 中
        MtlParser mtlParser;
        auto *material = mtlParser.ParseSingle(modelConfig.m_material.m_file, modelConfig.m_material.m_name);
        if (material == nullptr) {
            // MTL 解析失败或材质名未找到时，退回默认材质，避免后续空指针
            material = new Material();
            material->m_name = modelConfig.m_material.m_name;
            material->m_ambient_color = glm::vec3(0.2f);
            material->m_diffuse_color = glm::vec3(0.8f);
            material->m_specular_color = glm::vec3(0.0f);
            material->m_shininess = 0.0f;
        }
        std::cout << "material loaded from mtl: " << material->m_name
                  << " (Ka " << material->m_ambient_color.r << "," << material->m_ambient_color.g << "," << material->m_ambient_color.b
                  << " Kd " << material->m_diffuse_color.r << "," << material->m_diffuse_color.g << "," << material->m_diffuse_color.b
                  << " Ks " << material->m_specular_color.r << "," << material->m_specular_color.g << "," << material->m_specular_color.b
                  << " Ns " << material->m_shininess << ")" << std::endl;

        auto *effect = new TechniqueLight(
            "default",
            modelConfig.m_shader_vert_file,
            modelConfig.m_shader_frag_file
        );
        effect->SetLights(lights_raw);
        for (const auto &m: model_obj->GetMeshes()) {
            m->SetEffect(effect);
        }
        // 模型贴图（漫反射/法线）登记到 m_textures 统一释放；
        // 多个 mesh 可能共享同一纹理 ID，需去重防止重复 glDeleteTextures
        for (const auto &m: model_obj->GetMeshes()) {
            for (unsigned int texId: {m->GetTexture(), m->GetNormalMap()}) {
                if (texId != 0 && std::find(m_textures.begin(), m_textures.end(), texId) == m_textures.end()) {
                    m_textures.push_back(texId);
                }
            }
        }
        // 【调试】一次性打印每个模型的贴图 ID，验证 assimp 是否成功加载漫反射/法线贴图
        //（0 表示未加载；若加载了但渲染色黑，则为纹理上传/采样问题）
        for (const auto &m: model_obj->GetMeshes()) {
            std::cout << "[texture] model=" << modelConfig.m_name
                      << " diffuse=" << m->GetTexture()
                      << " normal=" << m->GetNormalMap() << std::endl;
        }
        // 材质移交模型持有：材质是逐绘制状态，由 Model::Draw 交给各 Mesh 上传
        model_obj->SetMaterial(std::unique_ptr<Material>(material));
        m_techniques.push_back(std::unique_ptr<Technique>(effect));

        // 记录模型初始渲染风格：按 world.yaml 指定的片元着色器文件名推断，
        // 供属性面板下拉框回显当前风格（与 GetModelStyle 的默认值 Lit 区分）
        RenderStyle initialStyle = RenderStyle::Lit;
        const std::string &fragFile = modelConfig.m_shader_frag_file;
        if (fragFile.find("toon") != std::string::npos) {
            initialStyle = RenderStyle::Toon;
        } else if (fragFile.find("textured") != std::string::npos) {
            initialStyle = RenderStyle::Textured;
        } else if (fragFile.find("unlit") != std::string::npos) {
            initialStyle = RenderStyle::Unlit;
        }
        m_model_styles[model_obj] = initialStyle;

        model_obj->SetScale(modelConfig.m_scale);
        model_obj->SetTranslate(modelConfig.m_position);
        model_obj->SetRotation(modelConfig.m_rotation.x, modelConfig.m_rotation.y, modelConfig.m_rotation.z);

        m_models.push_back(std::unique_ptr<Model>(model_obj));
    }

    // ---- 通用动画：按 world.yaml animations 配置创建，绑定到模型或灯光 ----
    // 目标对象按 light 优先、model 兜底解析；各配置通道统一打包为 AnimChannel 注册，
    // 目标不支持的通道（如方向光不支持位移）只告警跳过，不中断其它通道的绑定。
    for (const auto &animConfig: gConfig->m_animations) {
        IAnimTarget *target = animConfig.m_light_name.empty() ? nullptr : GetLight(animConfig.m_light_name);
        if (target == nullptr) {
            target = GetModel(animConfig.m_model_name);
        }
        if (target == nullptr) {
            const std::string &missing = animConfig.m_light_name.empty() ? animConfig.m_model_name : animConfig.m_light_name;
            std::cout << "[animation] skip " << animConfig.m_name
                      << ": model/light " << missing << " not found" << std::endl;
            continue;
        }
        auto *anim = CreateAnimation(target);
        anim->SetName(animConfig.m_name);
        anim->SetEnabled(animConfig.m_enabled);

        auto bind_channel = [&](AnimChannel ch, const char *ch_name) {
            if (!target->CanAnimate(ch.m_property)) {
                std::cout << "[animation] " << animConfig.m_name << ": " << ch_name
                          << " 通道不被目标 " << target->GetName() << " 支持，已跳过" << std::endl;
                return;
            }
            anim->AddChannel(ch);
        };

        if (animConfig.m_has_translate) {
            AnimChannel ch;
            ch.m_property = AnimProperty::Position;
            ch.m_curve = AnimCurveType::Sine;
            ch.center = animConfig.translate_center;
            ch.amplitude = animConfig.translate_amplitude;
            ch.m_frequency = animConfig.m_translate_frequency;
            bind_channel(ch, "translate");
        }
        if (animConfig.m_has_orbit) {
            // 圆周轨道：同属 Position 通道，曲线类型换 Orbit（x/z 半径形成圆周）
            AnimChannel ch;
            ch.m_property = AnimProperty::Position;
            ch.m_curve = AnimCurveType::Orbit;
            ch.center = animConfig.orbit_center;
            ch.amplitude = animConfig.orbit_radius;
            ch.m_frequency = animConfig.m_orbit_frequency;
            bind_channel(ch, "orbit");
        }
        if (animConfig.m_has_scale) {
            AnimChannel ch;
            ch.m_property = AnimProperty::Scale;
            ch.m_curve = AnimCurveType::Sine;
            ch.center = animConfig.scale_base;
            ch.amplitude = animConfig.scale_amplitude;
            ch.m_frequency = animConfig.m_scale_frequency;
            bind_channel(ch, "scale");
        }
        if (animConfig.m_has_rotate) {
            AnimChannel ch;
            ch.m_property = AnimProperty::Rotation;
            if (animConfig.m_rotate_spin) {
                // 持续旋转：从基准角以各轴 speed 度/秒匀速推进
                ch.m_curve = AnimCurveType::Spin;
                ch.center = animConfig.rotate_base;
                ch.speed = animConfig.rotate_speed;
            } else {
                // 正弦摆动：围绕基准角以振幅/频率往复
                ch.m_curve = AnimCurveType::Sine;
                ch.center = animConfig.rotate_base;
                ch.amplitude = animConfig.rotate_amplitude;
                ch.m_frequency = animConfig.m_rotate_frequency;
            }
            bind_channel(ch, "rotate");
        }
        if (animConfig.m_has_color) {
            AnimChannel ch;
            ch.m_property = AnimProperty::LightColor;
            ch.m_curve = AnimCurveType::Sine;
            ch.center = animConfig.color_center;
            ch.amplitude = animConfig.color_amplitude;
            ch.m_frequency = animConfig.m_color_frequency;
            bind_channel(ch, "color");
        }
        if (animConfig.m_has_intensity) {
            AnimChannel ch;
            ch.m_property = AnimProperty::LightIntensity;
            ch.m_curve = AnimCurveType::Sine;
            ch.center = animConfig.intensity_center;
            ch.amplitude = animConfig.intensity_amplitude;
            ch.m_frequency = animConfig.m_intensity_frequency;
            bind_channel(ch, "intensity");
        }
        std::cout << "[animation] " << animConfig.m_name << " -> " << target->GetName() << std::endl;
    }

    // ---- 渲染风格技术池（运行时切换，基础版：无参数调节）----
    // 为四套标准着色器（unlit/textured/lit/toon）各创建一个共享 Technique，
    // 运行时 SetModelStyle 遍历模型 mesh 换用对应技术，实现界面切换渲染风格。
    // 共享实例登记进 m_techniques 统一释放；材质由各 Model 自己持有，此处不再登记。
    auto *styleUnlit = new Technique("style_unlit",
                                     "./resource/shader/unlit.vert",
                                     "./resource/shader/unlit.frag");
    m_style_techniques[RenderStyle::Unlit] = styleUnlit;
    m_techniques.push_back(std::unique_ptr<Technique>(styleUnlit));

    auto *styleTextured = new Technique("style_textured",
                                        "./resource/shader/textured.vert",
                                        "./resource/shader/textured.frag");
    m_style_techniques[RenderStyle::Textured] = styleTextured;
    m_techniques.push_back(std::unique_ptr<Technique>(styleTextured));

    // lit/toon 需要接收材质与灯光：使用 TechniqueLight（支持 SetMaterial/SetLights），
    // 与 unlit/textured 的普通 Technique 区分开
    auto *styleLit = new TechniqueLight("style_lit",
                                        "./resource/shader/lit.vert",
                                        "./resource/shader/lit.frag");
    m_style_techniques[RenderStyle::Lit] = styleLit;
    m_techniques.push_back(std::unique_ptr<Technique>(styleLit));

    auto *styleToon = new TechniqueLight("style_toon",
                                         "./resource/shader/toon.vert",
                                         "./resource/shader/toon.frag");
    m_style_techniques[RenderStyle::Toon] = styleToon;
    m_techniques.push_back(std::unique_ptr<Technique>(styleToon));

    std::cout << "Setup world finish" << std::endl;
}

void Renderer::draw(long long elapsed) {
    // 计算投影竖切像机矩阵
    // 绘制帧数量加1
    m_fps_counter->Add();

    // 收集灯光非拥有指针视图，供各渲染 Pass 使用
    std::vector<Light *> scene_lights;
    scene_lights.reserve(m_lights.size());
    for (const auto &light : m_lights) {
        scene_lights.push_back(light.get());
    }

    // 视口背景色：默认深灰，调试面板 ColorEdit3 可实时修改（见 SetClearColor）
    glClearColor(m_clear_color.r, m_clear_color.g, m_clear_color.b, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_view_matrix = m_camera->GetViewMatrix();
    m_eye_pos = m_camera->GetPosition();

    // ---- 组装帧级渲染上下文（RenderContext）----
    // 整帧不变的渲染参数统一装入 ctx，沿 SkyDome/Terrain/Model/Mesh/Particle 绘制链贯通，
    // 阴影状态（ctx.shadow）在下方深度 Pass 分段更新，场景 Pass 直接读取。
    RenderContext ctx;
    ctx.m_elapsed = elapsed;
    ctx.m_projection = m_projection_matrix;
    ctx.m_view = m_view_matrix;
    ctx.m_camera = m_eye_pos;
    ctx.m_lights = scene_lights;
    // bias 缩放系数跟随面板滑块，每帧统一写入上下文
    ctx.m_shadow.m_bias_scale = m_shadow_pass->GetBiasScale();

    // ---- 方向光阴影深度 Pass ----
    // 计算光源摄像机并以光源视角重绘深度贴图，结果写入 ctx.shadow 供场景 Pass 采样
    m_shadow_pass->Render(m_models, m_terrain_manager.get(), ctx);

    // ---- 场景 Pass：所有 3D 内容渲染到 HDR 场景 FBO ----
    // 原实现直接画进默认帧缓冲；现在先画到 RGBA16F 离屏 FBO（保留 HDR 精度），
    // 结束后由后处理 Pass 统一做 tone mapping 输出到默认缓冲
    m_scene_fbo->BindForWrite();

    {
        // 天空穹最后绘制，关掉深度写入避免其遮住已绘制的几何
        GLDepthWriteGuard sky_depth_write;
        glDepthMask(GL_FALSE);
        m_sky_dome->Draw(ctx);
    }

    // 坐标轴 gizmo 不在 3D 场景中绘制：由 mainwindow 在 ImGui 绘制阶段
    // 以屏幕空间叠加方式渲染于视口角落（见 mainwindow.cpp 的 RenderFrame）

    // ---- 线框模式 ----
    // 开启时地形与模型以线框渲染（便于观察布线），天空穹/粒子保持原样：
    // 天空穹始终 FILL（半球线框会视觉污染背景），粒子是点图元不受 polygonMode 影响。
    // 阴影 Pass 在线框开关之前已结束，深度贴图始终以实体生成，不受影响。
    // 修改全局 GL 状态必须保存并在作用域结束恢复。
    // 【macOS 兼容】开关与恢复都必须整体用 GL_FRONT_AND_BACK 调用：
    //   Metal 后端（OpenGL 4.1 Metal）对 glPolygonMode(GL_FRONT/_BACK, ...) 单独设置
    //   face 的调用返回 GL_INVALID_ENUM 且状态不变。若恢复时分开设置 front/back，
    //   线框关闭后 GL_LINE 残留，连后处理全屏三角形也会退化为边线导致画面全黑。
    //   front/back 光栅化模式始终一致（本项目从未单面设置），故可直接取 front 值整体恢复。
    // guard 用 unique_ptr 持有：线框作用域结束处显式 reset 触发恢复，
    // 不能直接用函数作用域对象，否则会一直保留到函数末尾，
    // 连带后面的后处理全屏三角形也退化成线框、画面全黑。
    std::unique_ptr<GLPolygonModeGuard> polygon_mode;
    if (m_wireframe_enabled) {
        polygon_mode = std::make_unique<GLPolygonModeGuard>();
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // 绘制地形
    m_terrain_manager->Draw(ctx);

    // 网格地面辅助线：XZ 平面世界网格，独立于光源 gizmo 开关（见 DrawGrid）
    if (m_grid_enabled && m_debug_draw != nullptr) {
        DrawGrid();
        // 线框模式下不额外恢复状态：DebugDraw 是 GL_LINES，polygonMode 不影响线段图元
        m_debug_draw->Render(m_projection_matrix, m_view_matrix);
    }

    // 绘制配置的粒子系统
    for (const auto& ps: m_particle_systems) {
        ps->Draw(ctx, glm::mat4(1.0f));
    }

    if (m_debug_draw != nullptr) {
        m_light_gizmo.Render(*m_debug_draw, scene_lights, m_projection_matrix, m_view_matrix);
    }

    // 绘制模型
    for (const auto &m: m_models) {
        m->Draw(ctx, glm::mat4(1.0f));
    }

    // 法线可视化：在模型绘制之后收集所有模型顶点的法线线段，
    // 颜色按法线方向编码，便于检查法线朝向是否正确
    if (m_normal_visualization_enabled && m_debug_draw != nullptr) {
        CollectModelNormals();
        m_debug_draw->Render(m_projection_matrix, m_view_matrix);
    }

    // 鼠标拾取高亮：把命中的世界空间 AABB 画成线框盒叠加在场景之上
    // （DebugDraw 支持 GL_LINES，不受上方线框/FILL 多边形模式开关影响）；
    // 空盒（min==max）代表无高亮，直接跳过
    if (m_debug_draw != nullptr && m_pick_highlight_min != m_pick_highlight_max) {
        m_debug_draw->DrawBoxWireframe(m_pick_highlight_min, m_pick_highlight_max,
                                       glm::vec3(1.0f, 0.85f, 0.2f));
        m_debug_draw->Render(m_projection_matrix, m_view_matrix);
    }

    // 线框作用域结束：释放 guard 触发恢复，整体用 GL_FRONT_AND_BACK 写回
    // （macOS Metal 兼容性要求，见上方线框模式注释）
    polygon_mode.reset();

    // 场景 Pass 结束，回到默认帧缓冲（SceneFramebuffer 内部恢复主视口）
    m_scene_fbo->Unbind();

    // ---- 后处理 Pass：全屏三角形采样 HDR 场景纹理，做 tone mapping 后画到默认缓冲 ----
    m_post_process_pass->Render(m_scene_fbo->GetColorTexture());
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
    
    // 更新配置的变换动画（单位换算成秒，与粒子系统一致）
    for (const auto &anim: m_animations) {
        anim->Update(elapsed / 1000.0f);
    }

    // 更新配置的粒子系统
    for (const auto &ps: m_particle_systems) {
        ps->Update(elapsed / 1000.0f);
    }
}

/*
 * 创建并登记绑定到指定目标对象（模型/灯光等）的通用动画
 *
 * 动画所有权存入 m_animations（unique_ptr 容器），返回裸指针
 * 供调用方继续配置动画通道参数。目标指针仅为弱引用（不拥有）。
 */
Animation *Renderer::CreateAnimation(IAnimTarget *target) {
    if (target == nullptr) {
        return nullptr;
    }
    auto anim = std::make_unique<Animation>(target);
    auto *raw = anim.get();
    m_animations.push_back(std::move(anim));
    return raw;
}

Model *Renderer::GetModel(const std::string &name) {
    for (const auto &model: m_models) {
        if (model->GetName() == name) {
            return model.get();
        }
    }
    return nullptr;
}

Model *Renderer::GetModelByUUID(const std::string &uuid) {
    for (const auto &model: m_models) {
        if (model->GetUUID() == uuid) {
            return model.get();
        }
    }
    return nullptr;
}

/*
 * 运行时切换模型渲染风格（基础版，无参数调节）
 *
 * 思路（业界常见"共享技术池"做法）：四套标准着色器各持一个共享 Technique
 * （见 init 中 m_style_techniques），切换 = 把模型所有 mesh 的 effect 换成池中实例。
 * Mesh::Draw 每帧会重设变换矩阵/相机/灯光/阴影/材质等 uniform，故切换后下一帧自动生效。
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
    for (const auto &mesh: model->GetMeshes()) {
        if (mesh != nullptr) {
            mesh->SetEffect(it->second);
        }
    }
    m_model_styles[model] = style;
}

RenderStyle Renderer::GetModelStyle(Model *model) const {
    auto it = m_model_styles.find(model);
    if (it != m_model_styles.end()) {
        return it->second;
    }
    return RenderStyle::Lit; // 默认与 old.world.yaml 传统光照一致
}

Material *Renderer::GetModelMaterial(Model *model) const {
    return model == nullptr ? nullptr : model->GetMaterial();
}

/* 设置鼠标拾取结果的线框高亮盒（世界空间 AABB），draw 末尾叠加绘制 */
void Renderer::SetPickHighlight(const glm::vec3 &min, const glm::vec3 &max) {
    m_pick_highlight_min = min;
    m_pick_highlight_max = max;
}

/* 清除拾取高亮：置为空盒（min==max），draw 末尾检测空盒跳过绘制 */
void Renderer::ClearPickHighlight() {
    m_pick_highlight_min = glm::vec3(0.0f);
    m_pick_highlight_max = glm::vec3(0.0f);
}

Light *Renderer::GetLightByUUID(const std::string &uuid) const {
    for (const auto &light: m_lights) {
        if (light->GetUUID() == uuid) {
            return light.get();
        }
    }
    return nullptr;
}

Light *Renderer::GetLight(const std::string &name) {
    for (const auto &light: m_lights) {
        if (light->GetName() == name) {
            return light.get();
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
    m_camera = m_cameras[index].get();

    // 操控器重新绑定到新相机，并由新相机姿态反推轨道参数：
    // 每次切换后轨道中心落在新相机正前方（保持当前缩放级别），视角不跳变
    m_manipulator->SetCamera(m_camera);
    m_manipulator->ResetFromCamera();

    std::cout << "Camera switch to " << m_camera->GetName() << std::endl;
}

// 投影模式是摄像机属性：写入当前摄像机后立即重算投影矩阵（与 SetFov 相同模式），
// 切换摄像机时各自保持互不影响
void Renderer::SetProjectionType(ProjectionType type) {
    if (m_camera != nullptr) {
        m_camera->SetProjectionType(type);
        // 投影随相机属性即时生效，无需等窗口尺寸变化触发重算
        calculateProjectMatrix(width, height);
    }
}

ProjectionType Renderer::GetProjectionType() const {
    return (m_camera != nullptr) ? m_camera->GetProjectionType() : ProjectionType::Perspective;
}

unsigned int Renderer::GetShadowDepthTexture() const {
    return (m_shadow_pass != nullptr) ? m_shadow_pass->GetDepthTexture() : 0;
}

bool Renderer::IsShadowMapReady() const {
    return (m_shadow_pass != nullptr) && m_shadow_pass->IsMapReady();
}

void Renderer::SetShadowsEnabled(bool enabled) {
    if (m_shadow_pass != nullptr) {
        m_shadow_pass->SetEnabled(enabled);
    }
}

bool Renderer::IsShadowsEnabled() const {
    return (m_shadow_pass != nullptr) && m_shadow_pass->IsEnabled();
}

const ShadowCameraParams &Renderer::GetShadowCameraParams() const {
    // 引用返回无"空值"，用 available=false 的静态哨兵表示阴影 Pass 尚未创建
    static const ShadowCameraParams s_unavailable;
    return (m_shadow_pass != nullptr) ? m_shadow_pass->GetCameraParams() : s_unavailable;
}

void Renderer::SetShadowBiasScale(float scale) {
    if (m_shadow_pass != nullptr) {
        m_shadow_pass->SetBiasScale(scale);
    }
}

float Renderer::GetShadowBiasScale() const {
    return (m_shadow_pass != nullptr) ? m_shadow_pass->GetBiasScale() : 1.0f;
}

void Renderer::SetToneMappingEnabled(bool enabled) {
    if (m_post_process_pass != nullptr) {
        m_post_process_pass->SetToneMappingEnabled(enabled);
    }
}

bool Renderer::IsToneMappingEnabled() const {
    return (m_post_process_pass != nullptr) && m_post_process_pass->IsToneMappingEnabled();
}

void Renderer::SetExposure(float exposure) {
    if (m_post_process_pass != nullptr) {
        m_post_process_pass->SetExposure(exposure);
    }
}

float Renderer::GetExposure() const {
    return (m_post_process_pass != nullptr) ? m_post_process_pass->GetExposure() : 1.0f;
}

void Renderer::SetSaturation(float sat) {
    if (m_post_process_pass != nullptr) {
        m_post_process_pass->SetSaturation(sat);
    }
}

float Renderer::GetSaturation() const {
    return (m_post_process_pass != nullptr) ? m_post_process_pass->GetSaturation() : 1.0f;
}

void Renderer::SetContrast(float contrast) {
    if (m_post_process_pass != nullptr) {
        m_post_process_pass->SetContrast(contrast);
    }
}

float Renderer::GetContrast() const {
    return (m_post_process_pass != nullptr) ? m_post_process_pass->GetContrast() : 1.0f;
}

void Renderer::SetDebugDrawEnabled(bool enabled) {
    m_light_gizmo.SetEnabled(enabled);
}

bool Renderer::IsDebugDrawEnabled() const {
    return m_light_gizmo.IsEnabled();
}

void Renderer::SetLightRangeEnabled(bool enabled) {
    m_light_gizmo.SetRangeEnabled(enabled);
}

bool Renderer::IsLightRangeEnabled() const {
    return m_light_gizmo.IsRangeEnabled();
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
 * 网格仅与开关（m_grid_enabled）绑定，不受光源 gizmo 开关（SetDebugDrawEnabled）影响。
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
 * 法线线段长度使用统一的世界空间固定值（m_normal_length，ImGui 可调），
 * 保证所有模型法线等长、视觉一致；不再按网格包围盒比例计算，
 * 避免大模型法线过长、小模型法线过短的尺度差异。
 */
void Renderer::CollectModelNormals() {
    for (const auto &model: m_models) {
        if (model == nullptr) {
            continue;
        }
        // 模型矩阵直接复用 GetWorldMatrix()，保证与渲染几何一致
        // （含三轴欧拉角旋转，不再手动重建仅绕 Y 的矩阵）
        glm::mat4 modelMat = model->GetWorldMatrix();

        // 法线矩阵 = 模型矩阵的逆转置，用于把模型空间的法线变换到世界空间，
        // 保证非均匀缩放时法线方向正确（scale=1 时等价于 modelMat 的旋转部分）
        glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

        const auto &meshes = model->GetMeshes();
        for (const auto &mesh: meshes) {
            if (mesh == nullptr) {
                continue;
            }
            for (const auto &vertex: mesh->GetVertices()) {
                glm::vec3 worldPos = glm::vec3(modelMat * glm::vec4(vertex.m_position, 1.0f));
                glm::vec3 worldNormal = glm::normalize(normalMat * vertex.m_normal);
                // 统一长度（世界空间固定值），所有模型法线视觉等长
                m_debug_draw->DrawNormal(worldPos, worldNormal, m_normal_length);
            }
        }
    }
}

void Renderer::calculateProjectMatrix(const int w, const int h) {
    // 投影模式取自当前摄像机（相机属性）；相机未创建时回退透视
    const ProjectionType projection = (m_camera != nullptr)
                                          ? m_camera->GetProjectionType()
                                          : ProjectionType::Perspective;

    if (projection == ProjectionType::Perspective) {
        const float fov = m_fov; // 视野角度（member，调试面板可调）
        const float aspectRatio = (float) w / (float) (1 * h); // 宽高比
        const float nearPlane = gConfig->m_clip.m_clip_near; // 近平面距离
        const float farPlane = gConfig->m_clip.m_clip_far; // 远平面距离
        m_projection_matrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane); // 透视
    } else {
        // 正交投影：范围按地形地面尺寸自适应（半尺寸 + 20% 边距），
        // 顶/侧视图能看全整个场景；旧实现固定 ±20 只能看到场景中央一小块
        float sceneHalf = 60.0f; // 回退默认（地形管理器尚未创建时，对应默认 planeSize=100）
        if (m_terrain_manager != nullptr) {
            const TerrainConfig &cfg = m_terrain_manager->GetConfig();
            sceneHalf = cfg.m_plane_size * 0.5f + cfg.m_plane_size * 0.2f;
        }

        // 垂直范围 = ±sceneHalf，水平范围按窗口宽高比放大（长边看更多地面）
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        const float left = -sceneHalf * aspectRatio;
        const float right = sceneHalf * aspectRatio;
        const float bottom = -sceneHalf;
        const float top = sceneHalf;

        float nearPlane = gConfig->m_clip.m_clip_near; // 近平面距离
        float farPlane = gConfig->m_clip.m_clip_far; // 远平面距离
        m_projection_matrix = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    }
}
