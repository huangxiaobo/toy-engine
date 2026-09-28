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
    glm::vec3 m_position;           // 发射器位置
    float m_emit_rate;               // 每秒发射粒子数
    int m_max_particles;             // 最大粒子数
    
    // 粒子初始属性范围
    float m_min_life, m_max_life;       // 生命周期范围
    float m_min_size, m_max_size;       // 大小范围
    glm::vec3 m_min_velocity, m_max_velocity; // 速度范围
    float m_min_size_end, m_max_size_end; // 结束大小范围
    
    // 物理属性
    glm::vec3 m_gravity;            // 重力
    float m_drag;                   // 阻力
    
    // 方法
    void SetMaxParticles(int maxParticles);
    void Update(float deltaTime);
    void Emit();
    const std::vector<Particle>& GetParticles() const { return m_particles; }
    int GetAliveCount() const;
    
private:
    std::vector<Particle> m_particles;
    float m_emit_accumulator;      // 发射累积器
    
    // 随机数生成器
    std::random_device m_rd;
    std::mt19937 m_gen;
    
    float RandomFloat(float min, float max);
    glm::vec3 RandomVec3(const glm::vec3& min, const glm::vec3& max);
    int RandomInt(int min, int max);
    glm::vec3 RandomFireworkColor();
};

#endif
