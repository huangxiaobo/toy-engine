#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <vector>
#include <glm/glm.hpp>
#include "config.h"

using namespace std;

class Model;
class Axis;
class Light;
class Material;
class Technique;
class Camera;
class FPSCounter;
class TerrainManager;
class ParticleSystem;
class SkyDome;
class ShadowFramebuffer;
class SceneFramebuffer;
class DebugDraw;

enum class ProjectionType {
    Perspective,
    Orthographic
};

// 光源（阴影）摄像机参数
// 方向光阴影在深度 Pass 中用一个"光源视角的摄像机"把场景写进深度贴图。
// 这些参数每帧由 Renderer::draw 在阴影深度 Pass 前计算得出（见 renderer.cpp），
// 保存下来供 ImGui 阴影属性面板展示与调试（位置、朝向、正交投影范围、矩阵等）。
struct ShadowCameraParams {
    glm::vec3 position  = glm::vec3(0.0f);                 // 光源位置（光源摄像机所在位置，即 -方向 × 距离）
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);    // 光照方向（从光源指向场景，归一化）
    glm::vec3 lookAt    = glm::vec3(0.0f);                 // 注视目标（场景中心）
    glm::vec3 up        = glm::vec3(0.0f, 1.0f, 0.0f);     // lookAt 使用的 up 向量（预防平行退化的安全 up）
    float orthoLeft     = -210.0f;                         // 正交投影左边界
    float orthoRight    = 210.0f;                          // 正交投影右边界
    float orthoBottom   = -210.0f;                         // 正交投影下边界
    float orthoTop      = 210.0f;                          // 正交投影上边界
    float nearPlane     = 1.0f;                            // 近平面
    float farPlane      = 100.0f;                          // 远平面
    bool  available     = false;                           // 本帧是否存在已启用的方向光（参数是否有效）
    glm::mat4 lightView       = glm::mat4(1.0f);           // 光源视图矩阵（lightView）
    glm::mat4 lightProjection = glm::mat4(1.0f);           // 光源正交投影矩阵（lightProjection）
};

class Renderer {
public:
    explicit Renderer();

    virtual ~Renderer();

public:
    void init(int w, int h);

    void draw(long long elapsed);

    void resize(int w, int h);

    void update(long long elapsed);

public:
    // 获取模型数量
    int GetModelCount() const { return m_models.size(); }
    // 获取全部模型（引用返回，避免每帧拷贝）
    const std::vector<Model *> &GetModels() const { return m_models; }

    // 通过名字获取模型
    Model *GetModel(const string& name);

    // 通过uuid获取模型
    Model *GetModelByUUID(const string& uuid);

    // 获取全部灯光（引用返回，避免每帧拷贝）
    const std::vector<Light *> &GetLights() const { return m_lights; }

    // 通过uuid获取灯光
    Light *GetLightByUUID(const std::string &uuid) const;

    // 获取地形管理器（供 ImGui 属性面板访问配置和统计信息）
    TerrainManager *GetTerrainManager() const { return m_terrain_manager; }

    // 获取天空穹（供 ImGui 属性面板编辑颜色等属性）
    SkyDome *GetSkyDome() const { return m_sky_dome; }

    // 获取全部粒子系统（引用返回，避免每帧拷贝）
    const std::vector<ParticleSystem *> &GetParticleSystems() const { return m_particle_systems; }

    // 获取帧率
    float GetFPS() const;

    // 获取相机
    Camera *GetCamera() const { return m_camera; }

    // 获取屏幕空间坐标轴 gizmo（供 mainwindow 在 ImGui 绘制阶段叠加到视口角落）
    Axis *GetAxis() const { return m_axis; }
    
    // 获取所有摄像机配置
    const std::vector<Camera*>& GetCameras() const { return m_cameras; }
    
    // 切换到指定索引的摄像机
    void SwitchCamera(int index);

    // 切换视角
    void SerProjectionType(ProjectionType type);

    const ProjectionType GetProjectionType() const;

    // 获取本帧阴影深度贴图纹理 ID（供 ImGui 调试面板可视化）
    // 返回 0 表示阴影 FBO 尚未创建（阴影未启用 / 初始化前）
    unsigned int GetShadowDepthTexture() const;

    // 本帧是否已生成有效的阴影深度贴图（阴影 Pass 是否执行过）
    // 供调试面板判断当前可显示的深度贴图是否有实际内容
    bool IsShadowMapReady() const;

    // 阴影开关：禁用时跳过阴影深度 Pass，也不绑定阴影贴图
    void SetShadowsEnabled(bool enabled) { m_shadows_enabled = enabled; }
    bool IsShadowsEnabled() const { return m_shadows_enabled; }

    // 获取本帧光源（阴影）摄像机参数（供 ImGui 阴影属性面板展示与调试）
    const ShadowCameraParams &GetShadowCameraParams() const { return m_shadow_camera; }

    // 后处理 tone mapping 开关：关闭时全屏 Pass 直通输出（调试用），开启时做 Reinhard+gamma
    void SetToneMappingEnabled(bool enabled) { m_toneMappingEnabled = enabled; }
    bool IsToneMappingEnabled() const { return m_toneMappingEnabled; }

    // 光源调试可视化（DebugDraw gizmo）开关：禁用时跳过 gizmo 顶点收集与绘制
    void SetDebugDrawEnabled(bool enabled) { m_debugDrawEnabled = enabled; }
    bool IsDebugDrawEnabled() const { return m_debugDrawEnabled; }

private:
    void calculateProjectMatrix(int w, int h);

private:
    int width{};
    int height{};

    // 世界矩阵
    glm::mat4 m_projection_matrix{};
    glm::mat4 m_view_matrix{};
    glm::mat4 m_model_matrix{};
    glm::mat4 m_mvp_matrix{};
    glm::vec3 m_eye_pos{};

    ProjectionType m_projectionType = ProjectionType::Perspective;


    FPSCounter *m_fps_counter{};
    Axis *m_axis{};
    Camera* m_camera{};
    vector<Camera *> m_cameras;

    TerrainManager *m_terrain_manager{};
    SkyDome *m_sky_dome{};
    vector<ParticleSystem *> m_particle_systems;
    vector<Model *> m_models;
    // 光源位置/范围调试可视化系统（DebugDraw，方案 B），独立于 Model 体系
    DebugDraw *m_debug_draw = nullptr;
    // 光源调试可视化是否启用（ImGui 可配置，见 SetDebugDrawEnabled）
    bool m_debugDrawEnabled = true;
    vector<Light *> m_lights;

    // 渲染器创建并拥有的地形纹理，用于退出时统一释放
    unsigned int m_terrain_texture = 0;

    // 渲染器创建并拥有的模型纹理（如漫反射贴图与法线贴图），用于退出时统一释放
    vector<unsigned int> m_textures;

    // 渲染器创建并拥有的着色器技术（Technique），用于统一释放
    vector<Technique *> m_techniques;
    // 渲染器创建并拥有的材质（Material），用于统一释放
    vector<Material *> m_materials;

    // ---- 方向光阴影映射资源 ----
    // 阴影深度贴图 FBO（只写深度）
    ShadowFramebuffer *m_shadow_fbo = nullptr;
    // 深度 Pass 专用着色器（depth.vert/depth.frag）
    Technique *m_shadow_depth_tech = nullptr;
    // 光源空间矩阵（lightProjection * lightView），每帧由方向光计算后上传
    glm::mat4 m_light_space = glm::mat4(1.0f);
    // 本帧是否存在已生成的阴影深度贴图（决定主 Pass 是否启用阴影采样）
    bool m_shadow_map_ready = false;
    // 阴影是否启用（禁用时直接跳过深度 Pass，也不绑定阴影贴图）
    bool m_shadows_enabled = true;

    // 本帧光源（阴影）摄像机参数，深度 Pass 计算后保存，供 ImGui 面板展示
    ShadowCameraParams m_shadow_camera;

    // ---- HDR 场景帧缓冲 + 后处理（多 Pass 渲染框架）----
    // 所有 3D 场景绘制到该 FBO 的 RGBA16F 颜色纹理，后处理 Pass 再采样它做 tone mapping
    SceneFramebuffer *m_scene_fbo = nullptr;
    // 后处理全屏 Pass 着色器（post.vert/post.frag），输出到默认帧缓冲
    Technique *m_post_tech = nullptr;
    // 全屏三角形 VAO：无顶点属性绑定，仅满足 Core Profile 对 VAO 的强制要求
    unsigned int m_post_vao = 0;
    // tone mapping 是否启用（见 SetToneMappingEnabled）
    bool m_toneMappingEnabled = true;
};

#endif
