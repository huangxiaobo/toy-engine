#include "particle_system.h"
#include "particle_emitter.h"
#include "../technique/technique.h"
#include "../utils/utils.h"
#include "../utils/gl_state_guard.h"
#include "../render_context.h"
#include <glad/gl.h>
#include <iostream>

/*
 * 粒子系统（ParticleSystem）：CPU 模拟 + GPU 点精灵渲染
 *
 * 渲染路径：
 *   1. 每帧由 Update() 驱动发射器模拟物理并调用 UpdateBuffers()
 *   2. UpdateBuffers() 在 CPU 上把存活粒子打包成 ParticleVertex 数组，上传到 VBO
 *   3. Draw() 用 GL_POINTS（点精灵）一次性绘制全部粒子；gl_PointCoord 在片段着色器中
 *      实现圆形/纹理裁剪，gl_PointSize 控制屏幕上粒子大小
 *
 * 顶点布局（与 particle.vert 的 layout location 对应）：
 *   0: Position  模型空间位置
 *   1: Color     当前颜色（CPU 已按生命比例 mix 起始/结束色）
 *   2: Size      基础大小（像素）
 *   3: Life      剩余生命
 *   4: MaxLife   最大生命（用于计算生命比例）
 */
ParticleSystem::ParticleSystem()
    : m_emitter(nullptr)
    , m_effect(nullptr)
    , m_texture_id(0)
    , m_vao(0)
    , m_vbo(0)
    , m_vertex_count(0) {
}

ParticleSystem::~ParticleSystem() {
    if (m_emitter) {
        delete m_emitter;
        m_emitter = nullptr;
    }
    if (m_effect) {
        delete m_effect;
        m_effect = nullptr;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
    }
    // 释放粒子纹理
    if (m_texture_id != 0) {
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
    }
}

/*
 * 初始化粒子系统：发射器、着色器、纹理与 GPU 顶点缓冲
 *
 * 步骤：
 *   1. 创建发射器并给出默认粒子参数（寿命/大小/速度/颜色/重力/阻力）
 *   2. 加载粒子着色器（点精灵渲染由着色器内 gl_PointSize 控制）
 *   3. 创建棋盘格占位纹理，绑定到 sampler 供片段着色器采样
 *   4. 创建 VAO/VBO：按 MaxParticles 预分配缓冲（GL_DYNAMIC_DRAW，
 *      每帧以 glBufferSubData 增量更新）
 *   5. 设置 5 个顶点属性指针（布局见类注释）
 */
void ParticleSystem::Init(const glm::vec3& position) {
    // 创建发射器
    m_emitter = new ParticleEmitter();
    m_emitter->m_position = position;
    m_emitter->m_emit_rate = 50.0f;
    m_emitter->m_max_particles = 500;

    // 配置发射器属性
    m_emitter->m_min_life = 1.0f;
    m_emitter->m_max_life = 2.0f;
    m_emitter->m_min_size = 0.05f;
    m_emitter->m_max_size = 0.15f;
    m_emitter->m_min_velocity = glm::vec3(-0.5f, 1.0f, -0.5f);
    m_emitter->m_max_velocity = glm::vec3(0.5f, 3.0f, 0.5f);
    // 颜色无需在此配置：发射器按"烟花配色表"自动生成高饱和颜色，
    // 出生为白热核心、结束为同色相暗色（见 ParticleEmitter::RandomFireworkColor）
    m_emitter->m_min_size_end = 0.0f;
    m_emitter->m_max_size_end = 0.02f;
    m_emitter->m_gravity = glm::vec3(0.0f, -2.0f, 0.0f);
    m_emitter->m_drag = 0.98f;

    // 创建着色器
    m_effect = new Technique("particle",
                             "./resource/shader/particle.vert",
                             "./resource/shader/particle.frag");

    // 创建纹理
    m_texture_id = Utils::CreateCheckerboardTexture(64, 64, 8);

    // 创建VAO和VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // 预分配缓冲区：容量 = 最大粒子数 × 单粒子顶点大小
    // 之后每帧仅用 glBufferSubData 覆盖写，避免频繁 realloc 与 glBufferData 重建
    GLsizeiptr bufferSize = m_emitter->m_max_particles * sizeof(ParticleVertex);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);

    // 顶点属性布局（偏移量用 offsetof 计算，保证与 ParticleVertex 内存布局一致）
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)0);
    // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, m_color));
    // Size
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, m_size));
    // Life
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, m_life));
    // MaxLife
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, m_max_life));

    glBindVertexArray(0);
}

/*
 * 每帧更新粒子模拟，并把结果同步到 GPU 顶点缓冲
 *
 * 顺序：发射器推进物理（Update）→ 打包顶点并上传（UpdateBuffers）。
 * deltaTime 单位为秒。
 */
void ParticleSystem::Update(float deltaTime) {
    if (m_emitter) {
        m_emitter->Update(deltaTime);
        UpdateBuffers();
    }
}

/*
 * 绘制粒子（点精灵）
 *
 * 流程：
 *   1. 无存活粒子时直接跳过（避免无意义的绘制调用）
 *   2. 保存并开启混合（加法混合 src_alpha/one，实现发光叠加效果）、关闭深度写入
 *      （粒子叠加但互相无遮挡）、关闭背面剔除、开启程序点大小
 *   3. 激活着色器，绑定变换矩阵与粒子纹理
 *   4. glDrawArrays(GL_POINTS) 一次性绘制全部顶点
 *   5. 函数返回时 GLStateGuard 析构，自动恢复第 2 步改动的全部 GL 状态
 *
 * 注意：粒子物理由 Update 以秒为单位驱动，Draw 只负责渲染当前缓冲。
 */
void ParticleSystem::Draw(const RenderContext &ctx,
                          const glm::mat4& model) {
    if (!m_emitter || m_emitter->GetAliveCount() == 0) return;

    // 本函数要改写多项全局 GL 状态；用 guard 声明，函数返回时统一恢复，
    // 任何提前 return 或异常展开都不会把状态泄漏给后续场景绘制
    GLBlendGuard blend;
    GLBlendFuncGuard blendFunc;
    GLDepthWriteGuard depthWrite;
    GLCullFaceGuard cull;
    GLProgramPointSizeGuard pointSize;
    GLActiveTextureGuard activeTexture;

    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 加法混合

    // 禁用深度写入（但保持深度测试）
    glDepthMask(GL_FALSE);

    // 禁用背面剔除
    glDisable(GL_CULL_FACE);

    // 启用程序点大小
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 使用着色器
    m_effect->Enable();
    m_effect->SetProjectionMatrix(ctx.m_projection);
    m_effect->SetViewMatrix(ctx.m_view);
    m_effect->SetModelMatrix(model);

    // 绑定纹理到纹理单元0，并通知着色器 sampler
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    m_effect->SetUniform("particleTexture", 0);

    // 绘制粒子
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, m_vertex_count);
    glBindVertexArray(0);
}

/*
 * 按新的最大粒子数重新分配 VBO 容量
 *
 * 当世界配置（world.yaml）中 MaxParticles 大于默认值 500 时，
 * 需要在 Init 创建的缓冲区之外扩容，否则每帧 glBufferSubData 会越界。
 * 调用时机：配置加载后、首次渲染前。
 */
void ParticleSystem::ReallocateVBO() {
    if (!m_emitter) return;

    GLsizeiptr bufferSize = m_emitter->m_max_particles * sizeof(ParticleVertex);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
 * 打包存活粒子到 CPU 顶点数组，并上传 VBO
 *
 * 步骤：
 *   1. 遍历粒子池，仅将存活粒子写入 m_vertices（复用成员缓冲避免每帧堆分配）
 *   2. 颜色/大小按生命比例在 [起始值, 结束值] 之间插值（CPU 端 mix）
 *      —— 粒子出生时色亮大，随生命衰减变暗变小
 *   3. glBufferSubData 把打包结果写入 VBO 起始处（容量 ≥ 存活数，无需重建缓冲）
 */
void ParticleSystem::UpdateBuffers() {
    if (!m_emitter) return;

    const auto& particles = m_emitter->GetParticles();
    // 复用成员缓冲，避免每帧重新分配内存
    m_vertices.clear();
    m_vertices.reserve(m_emitter->GetAliveCount());

    for (const auto& p : particles) {
        if (!p.IsAlive()) continue;

        ParticleVertex vertex;
        vertex.m_position = p.m_position;
        vertex.m_color = glm::mix(p.m_color_end, p.m_color, p.GetLifeRatio());
        vertex.m_size = glm::mix(p.m_size_end, p.m_size, p.GetLifeRatio());
        vertex.m_life = p.m_life;
        vertex.m_max_life = p.m_max_life;
        m_vertices.push_back(vertex);
    }

    m_vertex_count = static_cast<int>(m_vertices.size());

    // 更新VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(ParticleVertex), m_vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}