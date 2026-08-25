#include "particle_emitter.h"
#include <algorithm>

ParticleEmitter::ParticleEmitter()
    : Position(0.0f)
    , EmitRate(100.0f)
    , MaxParticles(1000)
    , MinLife(1.0f)
    , MaxLife(3.0f)
    , MinSize(0.1f)
    , MaxSize(0.5f)
    , MinVelocity(-1.0f, 2.0f, -1.0f)
    , MaxVelocity(1.0f, 5.0f, 1.0f)
    , MinColor(1.0f, 0.5f, 0.0f)
    , MaxColor(1.0f, 1.0f, 0.0f)
    , MinColorEnd(1.0f, 0.0f, 0.0f)
    , MaxColorEnd(0.5f, 0.0f, 0.0f)
    , MinSizeEnd(0.0f)
    , MaxSizeEnd(0.1f)
    , Gravity(0.0f, -9.8f, 0.0f)
    , Drag(0.98f)
    , m_emitAccumulator(0.0f)
    , m_gen(m_rd()) {
    
    m_particles.resize(MaxParticles);
    for (auto& p : m_particles) {
        p.Life = 0.0f;
    }
}

ParticleEmitter::~ParticleEmitter() {
}

void ParticleEmitter::SetMaxParticles(int maxParticles) {
    int oldSize = m_particles.size();
    MaxParticles = maxParticles;
    m_particles.resize(MaxParticles);
    
    // 初始化新添加的粒子
    for (int i = oldSize; i < MaxParticles; i++) {
        m_particles[i].Life = 0.0f;
        m_particles[i].Age = 0.0f;
        m_particles[i].Position = glm::vec3(0.0f);
        m_particles[i].Velocity = glm::vec3(0.0f);
        m_particles[i].Color = glm::vec3(0.0f);
        m_particles[i].ColorEnd = glm::vec3(0.0f);
        m_particles[i].Size = 0.0f;
        m_particles[i].SizeEnd = 0.0f;
        m_particles[i].MaxLife = 0.0f;
    }
}

void ParticleEmitter::Update(float deltaTime) {
    // 先更新所有粒子生命
    for (auto& p : m_particles) {
        if (!p.IsAlive()) continue;
        
        p.Life -= deltaTime;
        p.Age += deltaTime;
        
        if (p.IsAlive()) {
            // 应用重力
            p.Velocity += Gravity * deltaTime;
            
            // 应用阻力
            p.Velocity *= Drag;
            
            // 更新位置
            p.Position += p.Velocity * deltaTime;
        }
    }
    
    // 然后发射新粒子（现在有死粒子可复用）
    m_emitAccumulator += EmitRate * deltaTime;
    while (m_emitAccumulator >= 1.0f) {
        Emit();
        m_emitAccumulator -= 1.0f;
    }
}

void ParticleEmitter::Emit() {
    // 查找一个死粒子
    for (auto& p : m_particles) {
        if (!p.IsAlive()) {
            // 初始化粒子
            p.Position = Position;
            p.Velocity = RandomVec3(MinVelocity, MaxVelocity);
            p.Color = RandomVec3(MinColor, MaxColor);
            p.ColorEnd = RandomVec3(MinColorEnd, MaxColorEnd);
            p.Size = RandomFloat(MinSize, MaxSize);
            p.SizeEnd = RandomFloat(MinSizeEnd, MaxSizeEnd);
            p.MaxLife = RandomFloat(MinLife, MaxLife);
            p.Life = p.MaxLife;
            p.Age = 0.0f;
            return;
        }
    }
}

int ParticleEmitter::GetAliveCount() const {
    int count = 0;
    for (const auto& p : m_particles) {
        if (p.IsAlive()) count++;
    }
    return count;
}

float ParticleEmitter::RandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_gen);
}

glm::vec3 ParticleEmitter::RandomVec3(const glm::vec3& min, const glm::vec3& max) {
    return glm::vec3(
        RandomFloat(min.x, max.x),
        RandomFloat(min.y, max.y),
        RandomFloat(min.z, max.z)
    );
}
