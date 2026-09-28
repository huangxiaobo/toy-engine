#ifndef __SHADOW_PASS_H__
#define __SHADOW_PASS_H__

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Model;
class TerrainManager;
class ShadowFramebuffer;
class Technique;
struct RenderContext;

/*
 * 光源（阴影）摄像机参数
 *
 * 方向光阴影在深度 Pass 中用一个"光源视角的摄像机"把场景写进深度贴图。
 * 这些参数每帧由 ShadowPass::Render 计算得出，保存下来供 ImGui 阴影属性面板
 * 展示与调试（位置、朝向、正交投影范围、矩阵等）。字段是矩阵计算的唯一参数来源，
 * 避免魔法数在计算处重复出现。
 */
struct ShadowCameraParams {
    glm::vec3 m_position  = glm::vec3(0.0f);                 // 光源位置（光源摄像机所在位置，即 -方向 × 距离）
    glm::vec3 m_direction = glm::vec3(0.0f, -1.0f, 0.0f);    // 光照方向（从光源指向场景，归一化）
    glm::vec3 m_look_at    = glm::vec3(0.0f);                 // 注视目标（场景中心）
    glm::vec3 m_up        = glm::vec3(0.0f, 1.0f, 0.0f);     // lookAt 使用的 up 向量（预防平行退化的安全 up）
    float m_ortho_left     = -210.0f;                         // 正交投影左边界
    float m_ortho_right    = 210.0f;                          // 正交投影右边界
    float m_ortho_bottom   = -210.0f;                         // 正交投影下边界
    float m_ortho_top      = 210.0f;                          // 正交投影上边界
    float m_near_plane     = 1.0f;                            // 近平面
    float m_far_plane      = 100.0f;                          // 远平面
    bool  m_available     = false;                           // 本帧是否存在已启用的方向光（参数是否有效）
    glm::mat4 m_light_view       = glm::mat4(1.0f);           // 光源视图矩阵（lightView）
    glm::mat4 m_light_projection = glm::mat4(1.0f);           // 光源正交投影矩阵（lightProjection）
};

/*
 * 方向光阴影深度 Pass
 *
 * 职责单一：先用"光源视角的摄像机"把场景写进阴影深度贴图，再把结果经
 * RenderContext::shadow 下发给场景 Pass 采样。深度 Pass 结束后 ctx.m_shadow.m_ready
 * 为真表示本帧有可采样的深度贴图；未启用或无方向光时保持 false，
 * 下游 Mesh::Draw 与地形自动跳过阴影采样。
 *
 * 提取自 Renderer::draw 原先内联的阴影段，把"每帧重算光源摄像机 + 深度重绘"
 * 这段与场景 Pass 无关的职责从编排函数里独立出来。
 */
class ShadowPass {
public:
    ShadowPass();
    ~ShadowPass();
    ShadowPass(const ShadowPass &) = delete;
    ShadowPass &operator=(const ShadowPass &) = delete;

    // 创建阴影深度贴图 FBO 与深度 Pass 专用着色器
    void Init();

    /*
     * 执行本帧阴影深度 Pass
     *
     * models  - 场景模型（只读，深度 Pass 以单位矩阵重绘它们的网格）
     * terrain - 地形管理器，用于把地面范围并入阴影正交盒（可为 nullptr）
     * ctx     - 帧上下文，灯光从 ctx.m_lights 读取，阴影结果写回 ctx.m_shadow
     */
    void Render(const std::vector<std::unique_ptr<Model>> &models,
                const TerrainManager *terrain,
                RenderContext &ctx);

    bool IsMapReady() const { return m_map_ready; }
    unsigned int GetDepthTexture() const;
    const ShadowCameraParams &GetCameraParams() const { return m_camera; }

    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    float GetBiasScale() const { return m_bias_scale; }
    void SetBiasScale(float scale) { m_bias_scale = scale; }

private:
    // 场景包围盒：地形地面范围为基准，并入所有模型顶点世界坐标；都没有时回退旧默认 ±210
    void computeSceneBounds(const std::vector<std::unique_ptr<Model>> &models,
                            const TerrainManager *terrain,
                            glm::vec3 &outMin, glm::vec3 &outMax) const;

private:
    // 阴影深度贴图 FBO（只写深度）
    std::unique_ptr<ShadowFramebuffer> m_fbo;
    // 深度 Pass 专用着色器（depth.vert/depth.frag）
    std::unique_ptr<Technique> m_depth_tech;

    // 本帧光源摄像机参数，深度 Pass 计算后保存，供 ImGui 面板展示
    ShadowCameraParams m_camera;

    // 本帧是否存在已生成的阴影深度贴图（决定场景 Pass 是否启用阴影采样）
    bool m_map_ready = false;
    // 阴影是否启用（禁用时直接跳过深度 Pass，也不绑定阴影贴图）
    bool m_enabled = true;
    // 阴影 bias 缩放系数（1.0 = 着色器原始公式）
    float m_bias_scale = 1.0f;
};

#endif
