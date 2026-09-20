#ifndef __RENDER_CONTEXT_H__
#define __RENDER_CONTEXT_H__

#include <glm/glm.hpp>
#include <vector>

class Light;
class Technique;

/*
 * 阴影帧状态（ShadowState）
 *
 * 每帧由 Renderer 组装，统一驱动所有几何体的阴影行为：
 *   - 深度 Pass 阶段：Model/Mesh 只写深度（depthTech + lightSpace）
 *   - 场景 Pass 阶段：模型与地形读取 ready/depthTexture/lightSpace 采样阴影
 *
 * 取代原先「Mesh 文件级静态量 + Terrain 逐层转发」两套并行的阴影状态通道，
 * 让阴影参数只经过 RenderContext 一条链路，避免两处状态不同步的问题。
 */
struct ShadowState {
    // 深度 Pass 专用着色器（depth.vert/depth.frag），passActive 时为 Mesh::Draw 的深度路径
    Technique *depthTech = nullptr;
    // 光源空间矩阵（正交投影 × 光源视图），深度 Pass 与场景 Pass 共用
    glm::mat4 lightSpace = glm::mat4(1.0f);
    // 当前是否处于阴影深度 Pass（Mesh::Draw 据此短路走只写深度分支）
    bool passActive = false;
    // 本帧是否存在可采样的阴影深度贴图（场景 Pass 是否启用阴影采样）
    bool ready = false;
    // 阴影深度贴图纹理ID（场景 Pass 绑定到纹理单元2）
    unsigned int depthTexture = 0;
    // 阴影 bias 缩放系数（1.0 = 着色器原始公式，随面板滑块调整）
    float biasScale = 1.0f;
};

/*
 * 帧级渲染上下文（RenderContext）
 *
 * 汇总整帧不变的渲染参数，沿 SkyDome / TerrainChunk / Model / Mesh / ParticleSystem
 * 的对象绘制链逐层传递，取代原先逐层展开的长参数列表
 * （elapsed/projection/view/camera/lights）。
 *
 * 由 Renderer::draw 每帧构建一次（值拷贝，仅栈上临时量），
 * 各 Draw 以 const 引用接收，不额外分配、不生命周期共享。
 * 阴影状态（shadow）在帧内会有两阶段变化（深度 Pass / 场景 Pass），
 * 因此不作为成员缓存，而是跟随本上下文实时更新。
 */
struct RenderContext {
    long long elapsed = 0;                  // 经过时间（毫秒，粒子动画用）
    glm::mat4 projection = glm::mat4(1.0f); // 相机投影矩阵
    glm::mat4 view = glm::mat4(1.0f);       // 相机视图矩阵
    glm::vec3 camera = glm::vec3(0.0f);     // 相机世界位置
    std::vector<Light *> lights;            // 本帧启用的灯光（非拥有）
    ShadowState shadow;                     // 本帧阴影状态（Renderer 实时填充）
};

#endif // __RENDER_CONTEXT_H__