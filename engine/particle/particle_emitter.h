#ifndef __PARTICLE_EMITTER_H__
#define __PARTICLE_EMITTER_H__

#include <glm/glm.hpp>
#include <vector>
#include <random>
#include "particle.h"

class ParticleEmitter {
public:
    ParticleEmitter();
    ~ParticleEmitter();
    
    // 发射器配置
    glm::vec3 Position;           // 发射器位置
    float EmitRate;               // 每秒发射粒子数
    int MaxParticles;             // 最大粒子数
    
    // 粒子初始属性范围
    float MinLife, MaxLife;       // 生命周期范围
    float MinSize, MaxSize;       // 大小范围
    glm::vec3 MinVelocity, MaxVelocity; // 速度范围
    glm::vec3 MinColor, MaxColor; // 颜色范围
    glm::vec3 MinColorEnd, MaxColorEnd; // 结束颜色范围
    float MinSizeEnd, MaxSizeEnd; // 结束大小范围
    
    // 物理属性
    glm::vec3 Gravity;            // 重力
    float Drag;                   // 阻力
    
    // 方法
    void SetMaxParticles(int maxParticles);
    void Update(float deltaTime);
    void Emit();
    const std::vector<Particle>& GetParticles() const { return m_particles; }
    int GetAliveCount() const;
    
private:
    std::vector<Particle> m_particles;
    float m_emitAccumulator;      // 发射累积器
    
    // 随机数生成器
    std::random_device m_rd;
    std::mt19937 m_gen;
    
    float RandomFloat(float min, float max);
    glm::vec3 RandomVec3(const glm::vec3& min, const glm::vec3& max);
};

#endif
