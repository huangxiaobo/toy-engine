#include "particle_system.h"
#include "particle_emitter.h"
#include "../technique/technique.h"
#include "../utils/utils.h"
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
    , m_textureID(0)
    , m_VAO(0)
    , m_VBO(0)
    , m_vertexCount(0) {
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
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
    }
    if (m_VBO) {
        glDeleteBuffers(1, &m_VBO);
    }
    // 释放粒子纹理
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
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
    m_emitter->Position = position;
    m_emitter->EmitRate = 50.0f;
    m_emitter->MaxParticles = 500;

    // 配置发射器属性
    m_emitter->MinLife = 1.0f;
    m_emitter->MaxLife = 2.0f;
    m_emitter->MinSize = 0.05f;
    m_emitter->MaxSize = 0.15f;
    m_emitter->MinVelocity = glm::vec3(-0.5f, 1.0f, -0.5f);
    m_emitter->MaxVelocity = glm::vec3(0.5f, 3.0f, 0.5f);
    m_emitter->MinColor = glm::vec3(1.0f, 0.6f, 0.0f);
    m_emitter->MaxColor = glm::vec3(1.0f, 1.0f, 0.2f);
    m_emitter->MinColorEnd = glm::vec3(1.0f, 0.0f, 0.0f);
    m_emitter->MaxColorEnd = glm::vec3(0.8f, 0.2f, 0.0f);
    m_emitter->MinSizeEnd = 0.0f;
    m_emitter->MaxSizeEnd = 0.02f;
    m_emitter->Gravity = glm::vec3(0.0f, -2.0f, 0.0f);
    m_emitter->Drag = 0.98f;

    // 创建着色器
    m_effect = new Technique("particle",
                             "./resource/shader/particle.vert",
                             "./resource/shader/particle.frag");

    // 创建纹理
    m_textureID = Utils::CreateCheckerboardTexture(64, 64, 8);

    // 创建VAO和VBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // 预分配缓冲区：容量 = 最大粒子数 × 单粒子顶点大小
    // 之后每帧仅用 glBufferSubData 覆盖写，避免频繁 realloc 与 glBufferData 重建
    GLsizeiptr bufferSize = m_emitter->MaxParticles * sizeof(ParticleVertex);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);

    // 顶点属性布局（偏移量用 offsetof 计算，保证与 ParticleVertex 内存布局一致）
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)0);
    // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, Color));
    // Size
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, Size));
    // Life
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, Life));
    // MaxLife
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, MaxLife));

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
 *   5. 恢复第 2 步保存的 GL 状态，避免影响后续场景绘制（见 AGENTS.md 经验 #2）
 *
 * 注意：elapsed 为毫秒，粒子物理由 Update 以秒为单位驱动。
 */
void ParticleSystem::Draw(long long elapsed,
                          const glm::mat4& projection,
                          const glm::mat4& view,
                          const glm::mat4& model,
                          const glm::vec3& camera,
                          const std::vector<Light*>& lights) {
    if (!m_emitter || m_emitter->GetAliveCount() == 0) return;

    // 保存当前OpenGL状态
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    GLboolean depthWriteEnabled;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
    GLboolean programPointSizeEnabled;
    glGetBooleanv(GL_PROGRAM_POINT_SIZE, &programPointSizeEnabled);
    GLboolean cullFaceEnabled;
    glGetBooleanv(GL_CULL_FACE, &cullFaceEnabled);

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
    m_effect->SetProjectionMatrix(projection);
    m_effect->SetViewMatrix(view);
    m_effect->SetModelMatrix(model);

    // 绑定纹理到纹理单元0，并通知着色器 sampler
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    m_effect->SetUniform("particleTexture", 0);

    // 绘制粒子
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
    glBindVertexArray(0);

    // 恢复OpenGL状态
    if (!blendEnabled) glDisable(GL_BLEND);
    glDepthMask(depthWriteEnabled ? GL_TRUE : GL_FALSE);
    if (!programPointSizeEnabled) glDisable(GL_PROGRAM_POINT_SIZE);
    if (!cullFaceEnabled) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
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

    GLsizeiptr bufferSize = m_emitter->MaxParticles * sizeof(ParticleVertex);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
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
        vertex.Position = p.Position;
        vertex.Color = glm::mix(p.ColorEnd, p.Color, p.GetLifeRatio());
        vertex.Size = glm::mix(p.SizeEnd, p.Size, p.GetLifeRatio());
        vertex.Life = p.Life;
        vertex.MaxLife = p.MaxLife;
        m_vertices.push_back(vertex);
    }

    m_vertexCount = static_cast<int>(m_vertices.size());

    // 更新VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(ParticleVertex), m_vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}