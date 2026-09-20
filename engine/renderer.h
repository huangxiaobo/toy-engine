#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <map>
#include "config.h"
#include "camera/camera.h" // ProjectionType 为相机属性，枚举定义随相机头文件

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
class OrbitManipulator;

// 运行时可切换的模型渲染风格（RenderStyle）
// 对应 resource/shader 下的四套标准着色器（unlit/textured/lit/toon）。
// 基础版：不含 Toon 参数调节滑块，仅切换着色器。
enum class RenderStyle {
    Unlit,    // 纯色：仅顶点颜色（unlit.vert/.frag）
    Textured, // 顶点颜色 × 漫反射贴图（textured.vert/.frag，无贴图时退化纯色）
    Lit,      // 材质 Blinn-Phong + 阴影 + 法线贴图（lit.vert/.frag）
    Toon      // 卡通渲染（toon.vert/.frag）
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
    // 获取全部模型（引用返回，避免拷贝）
    const std::vector<std::unique_ptr<Model>> &GetModels() const { return m_models; }

    // 通过名字获取模型
    Model *GetModel(const std::string &name);

    // 通过uuid获取模型
    Model *GetModelByUUID(const std::string &uuid);

    // 获取全部灯光（引用返回，避免每帧拷贝）
    const std::vector<std::unique_ptr<Light>> &GetLights() const { return m_lights; }

    // 通过uuid获取灯光
    Light *GetLightByUUID(const std::string &uuid) const;

    // 获取地形管理器（供 ImGui 属性面板访问配置和统计信息）
    TerrainManager *GetTerrainManager() const { return m_terrain_manager.get(); }

    // 获取天空穹（供 ImGui 属性面板编辑颜色等属性）
    SkyDome *GetSkyDome() const { return m_sky_dome.get(); }

    // 获取全部粒子系统（引用返回，避免每帧拷贝）
    const std::vector<std::unique_ptr<ParticleSystem>> &GetParticleSystems() const { return m_particle_systems; }

    // 获取帧率
    float GetFPS() const;

    // 获取相机
    Camera *GetCamera() const { return m_camera; }

    // 获取当前投影矩阵（供鼠标拾取等需要与渲染一致的空间换算复用）
    const glm::mat4 &GetProjectionMatrix() const { return m_projection_matrix; }

    // 获取当前视图矩阵（与 GetProjectionMatrix 配套，供拾取反投影使用）
    const glm::mat4 &GetViewMatrix() const { return m_view_matrix; }

    // 获取当前相机操控器（相机交互逻辑由操控器承载，与相机状态分离）
    OrbitManipulator *GetManipulator() const { return m_manipulator.get(); }

    // 获取屏幕空间坐标轴 gizmo（供 mainwindow 在 ImGui 绘制阶段叠加到视口角落）
    Axis *GetAxis() const { return m_axis.get(); }
    
    // 获取所有摄像机配置
    const std::vector<std::unique_ptr<Camera>> &GetCameras() const { return m_cameras; }
    
    // 切换到指定索引的摄像机
    void SwitchCamera(int index);

    // 切换当前摄像机的投影模式（透视/正交）：写入相机属性并立即重算投影矩阵
    void SetProjectionType(ProjectionType type);

    ProjectionType GetProjectionType() const;

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

    // 后处理 tone mapping 开关：关闭时全屏 Pass 直通输出（调试用），开启时做 ACES+gamma
    void SetToneMappingEnabled(bool enabled) { m_tone_mapping_enabled = enabled; }
    bool IsToneMappingEnabled() const { return m_tone_mapping_enabled; }

    // 后处理曝光系数（>1 提亮 / <1 压暗），ACES 映射前乘入 HDR 线性值，运行时可调
    void SetExposure(float exposure) { m_exposure = exposure; }
    float GetExposure() const { return m_exposure; }

    // 后处理饱和度/对比度（在 tonemap+gamma 后调整），默认 1.0 不调整，运行时可调
    void SetSaturation(float sat) { m_saturation = sat; }
    float GetSaturation() const { return m_saturation; }
    void SetContrast(float contrast) { m_contrast = contrast; }
    float GetContrast() const { return m_contrast; }

    // 阴影 bias 缩放系数（1.0 = 原默认），阴影范围自适应后需现场微调痤疮表现
    void SetShadowBiasScale(float scale) { m_shadow_bias_scale = scale; }
    float GetShadowBiasScale() const { return m_shadow_bias_scale; }

    // 光源调试可视化（DebugDraw gizmo）开关：禁用时跳过 gizmo 顶点收集与绘制
    void SetDebugDrawEnabled(bool enabled) { m_debug_draw_enabled = enabled; }
    bool IsDebugDrawEnabled() const { return m_debug_draw_enabled; }

    // 线框模式开关：启用时场景 Pass 中地形与模型以线框渲染（阴影 Pass 不受影响）
    void SetWireframeEnabled(bool enabled) { m_wireframe_enabled = enabled; }
    bool IsWireframeEnabled() const { return m_wireframe_enabled; }

    // 网格地面辅助线开关：XZ 平面世界网格（复用 DebugDraw 线段管线），帮助判断空间方位
    void SetGridEnabled(bool enabled) { m_grid_enabled = enabled; }
    bool IsGridEnabled() const { return m_grid_enabled; }

    // 视口背景色（glClearColor 的 RGB），运行时修改立即生效
    void SetClearColor(const glm::vec3 &color) { m_clear_color = color; }
    const glm::vec3 &GetClearColor() const { return m_clear_color; }

    // 视野角度（度）：修改后立即重算投影矩阵，运行时可调
    void SetFov(float fov);
    float GetFov() const { return m_fov; }

    // 法线可视化开关：启用时模型以法线方向着色（RGB = XYZ），便于检查法线方向是否正确
    void SetNormalVisualizationEnabled(bool enabled) { m_normal_visualization_enabled = enabled; }
    bool IsNormalVisualizationEnabled() const { return m_normal_visualization_enabled; }

    // 法线线段统一长度（世界空间单位），对所有模型生效（ImGui 可调）
    void SetNormalLength(float len) { m_normal_length = len; }
    float GetNormalLength() const { return m_normal_length; }

    // 光照范围可视化开关：启用时点光源/聚光灯显示影响范围球体/锥体线框
    void SetLightRangeEnabled(bool enabled) { m_light_range_enabled = enabled; }
    bool IsLightRangeEnabled() const { return m_light_range_enabled; }

    // 运行时切换模型渲染风格（基础版，无参数调节）
    // 遍历模型所有 mesh 换成风格池中对应 Technique；切换后下一帧自动生效。
    void SetModelStyle(Model *model, RenderStyle style);
    // 查询模型当前渲染风格（默认 Lit，与 world.yaml 默认行为一致）
    RenderStyle GetModelStyle(Model *model) const;
    // 查询模型的材质指针（属性面板显示/编辑用，Mesh 上的 Technique 可能为共享
    // 风格技术，其内部 m_material 会被其它模型覆盖，故必须按模型单独登记）
    Material *GetModelMaterial(Model *model) const;

    // ---- 拾取高亮 ----
    // 设置鼠标拾取结果的线框高亮盒（世界空间 AABB），下一帧生效；传入空盒清除高亮
    void SetPickHighlight(const glm::vec3 &min, const glm::vec3 &max);
    // 清除拾取高亮（与 SetPickHighlight 传空盒等价，语义更明确）
    void ClearPickHighlight();

private:
    void calculateProjectMatrix(int w, int h);
    // 收集并提交世界网格辅助线（XZ 平面，覆盖地形范围），由 draw 中网格开关控制
    void DrawGrid();
    // 收集模型法线线段到 DebugDraw（将顶点世界坐标与法线变换到世界空间）
    void CollectModelNormals();
    // 计算场景世界空间 AABB（地形范围 + 全部模型几何），供阴影正交范围自适应
    void computeSceneBounds(glm::vec3 &outMin, glm::vec3 &outMax) const;

private:
    int width{};
    int height{};

    // 世界矩阵
    glm::mat4 m_projection_matrix{};
    glm::mat4 m_view_matrix{};
    glm::vec3 m_eye_pos{};

    std::unique_ptr<FPSCounter> m_fps_counter;
    std::unique_ptr<Axis> m_axis;
    // m_camera 指向 m_cameras 容器内对象（非拥有别名），相机所有权归容器
    Camera *m_camera = nullptr;
    std::vector<std::unique_ptr<Camera>> m_cameras;
    // 当前相机操控器：负责把输入转换为相机姿态变化（轨道/平移/缩放），
    // 与相机状态分离（业界 Camera-Manipulator 分层）。由构造时创建并绑定 m_camera。
    std::unique_ptr<OrbitManipulator> m_manipulator;

    std::unique_ptr<TerrainManager> m_terrain_manager;
    std::unique_ptr<SkyDome> m_sky_dome;
    std::vector<std::unique_ptr<ParticleSystem>> m_particle_systems;
    std::vector<std::unique_ptr<Model>> m_models;
    // 光源位置/范围调试可视化系统（DebugDraw，方案 B），独立于 Model 体系
    std::unique_ptr<DebugDraw> m_debug_draw;
    // 光源调试可视化是否启用（ImGui 可配置，见 SetDebugDrawEnabled）
    bool m_debug_draw_enabled = false;
    // 线框模式是否启用（ImGui 可配置，见 SetWireframeEnabled）
    bool m_wireframe_enabled = false;
    // 网格地面辅助线是否启用（ImGui 可配置，见 SetGridEnabled）
    bool m_grid_enabled = false;
    // 视口背景色（glClearColor RGB），默认与旧硬编码值一致（深灰）
    glm::vec3 m_clear_color = glm::vec3(0.2f);
    // 视野角度（度），初始取自 config,运行时可调（见 SetFov）
    float m_fov = 45.0f;
    // 法线可视化是否启用（ImGui 可配置，见 SetNormalVisualizationEnabled）
    bool m_normal_visualization_enabled = false;
    // 法线线段统一长度（世界空间单位），默认 2.0，ImGui 可调（见 SetNormalLength）
    float m_normal_length = 2.0f;
    // 光照范围可视化是否启用（ImGui 可配置，见 SetLightRangeEnabled）
    bool m_light_range_enabled = false;
    std::vector<std::unique_ptr<Light>> m_lights;

    // 渲染器创建并拥有的地形纹理，用于退出时统一释放
    unsigned int m_terrain_texture = 0;

    // 渲染器创建并拥有的模型纹理（如漫反射贴图与法线贴图），用于退出时统一释放
    std::vector<unsigned int> m_textures;

    // 渲染器创建并拥有的着色器技术（Technique），用于统一释放
    std::vector<std::unique_ptr<Technique>> m_techniques;
    // 渲染器创建并拥有的材质（Material），用于统一释放
    std::vector<std::unique_ptr<Material>> m_materials;

    // ---- 运行时渲染风格切换（基础版）----
    // 风格池：四套标准着色器各持一个共享 Technique，运行时通过 SetModelStyle 换给模型 mesh
    std::map<RenderStyle, Technique *> m_style_techniques;
    // 模型 → 当前风格（默认 Lit），供 UI 下拉框回显当前选项
    std::map<Model *, RenderStyle> m_model_styles;
    // 模型 → 材质指针（仅引用不拥有）。风格技术为多模型共享实例，其内部材质会被
    // 交叉覆盖，属性面板必须按模型查自己的材质（见 GetModelMaterial）
    std::map<Model *, Material *> m_model_materials;

    // ---- 方向光阴影映射资源 ----
    // 阴影深度贴图 FBO（只写深度）
    std::unique_ptr<ShadowFramebuffer> m_shadow_fbo;
    // 深度 Pass 专用着色器（depth.vert/depth.frag）
    Technique *m_shadow_depth_tech = nullptr;
    // 光源空间矩阵（lightProjection * lightView），每帧由方向光计算后上传
    glm::mat4 m_light_space = glm::mat4(1.0f);
    // 本帧是否存在已生成的阴影深度贴图（决定主 Pass 是否启用阴影采样）
    bool m_shadow_map_ready = false;
    // 阴影是否启用（禁用时直接跳过深度 Pass，也不绑定阴影贴图）
    bool m_shadows_enabled = true;
    // 阴影 bias 缩放系数（默认 1.0，见 SetShadowBiasScale）
    float m_shadow_bias_scale = 1.0f;

    // 本帧光源（阴影）摄像机参数，深度 Pass 计算后保存，供 ImGui 面板展示
    ShadowCameraParams m_shadow_camera;

    // ---- HDR 场景帧缓冲 + 后处理（多 Pass 渲染框架）----
    // 所有 3D 场景绘制到该 FBO 的 RGBA16F 颜色纹理，后处理 Pass 再采样它做 tone mapping
    std::unique_ptr<SceneFramebuffer> m_scene_fbo;
    // 后处理全屏 Pass 着色器（post.vert/post.frag），输出到默认帧缓冲
    Technique *m_post_tech = nullptr;
    // 全屏三角形 VAO：无顶点属性绑定，仅满足 Core Profile 对 VAO 的强制要求
    unsigned int m_post_vao = 0;
    // tone mapping 是否启用（见 SetToneMappingEnabled）
    bool m_tone_mapping_enabled = true;
    // 后处理曝光系数（默认 1.0，见 SetExposure）
    float m_exposure = 1.0f;
    // 后处理饱和度（默认 1.0 不调整，见 SetSaturation）
    float m_saturation = 1.0f;
    // 后处理对比度（默认 1.0 不调整，见 SetContrast）
    float m_contrast = 1.0f;

    // ---- 拾取高亮状态 ----
    // 鼠标拾取结果的线框高亮盒（世界空间 AABB），draw 末尾用 DebugDraw 叠加绘制
    // 空盒（min==max）表示无高亮
    glm::vec3 m_pick_highlight_min = glm::vec3(0.0f);
    glm::vec3 m_pick_highlight_max = glm::vec3(0.0f);
};

#endif
