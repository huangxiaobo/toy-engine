#include "particle_system.h"
#include "particle_emitter.h"
#include "../technique/technique.h"
#include "../utils/utils.h"
#include <glad/gl.h>
#include <iostream>

// 粒子顶点数据结构
struct ParticleVertex {
    glm::vec3 Position;
    glm::vec3 Color;
    float Size;
    float Life;
    float MaxLife;
};

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
}

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
    
    // 预分配缓冲区
    GLsizeiptr bufferSize = m_emitter->MaxParticles * sizeof(ParticleVertex);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    
    // 顶点属性
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

void ParticleSystem::Update(float deltaTime) {
    if (m_emitter) {
        m_emitter->Update(deltaTime);
        UpdateBuffers();
        
        // 调试输出
        static int frameCount = 0;
        if (frameCount++ % 60 == 0) {
            std::cout << "Particles alive: " << m_emitter->GetAliveCount() << std::endl;
        }
    }
}

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
    
    // 绑定纹理
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

void ParticleSystem::ReallocateVBO() {
    if (!m_emitter) return;
    
    GLsizeiptr bufferSize = m_emitter->MaxParticles * sizeof(ParticleVertex);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::UpdateBuffers() {
    if (!m_emitter) return;
    
    const auto& particles = m_emitter->GetParticles();
    std::vector<ParticleVertex> vertices;
    vertices.reserve(m_emitter->GetAliveCount());
    
    for (const auto& p : particles) {
        if (!p.IsAlive()) continue;
        
        ParticleVertex vertex;
        vertex.Position = p.Position;
        vertex.Color = glm::mix(p.ColorEnd, p.Color, p.GetLifeRatio());
        vertex.Size = glm::mix(p.SizeEnd, p.Size, p.GetLifeRatio());
        vertex.Life = p.Life;
        vertex.MaxLife = p.MaxLife;
        vertices.push_back(vertex);
    }
    
    m_vertexCount = vertices.size();
    
    // 更新VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(ParticleVertex), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
