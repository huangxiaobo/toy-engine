#ifndef __PARTICLE_H__
#define __PARTICLE_H__

#include <glm/glm.hpp>

struct Particle {
    glm::vec3 m_position;      // 位置
    glm::vec3 m_velocity;      // 速度
    glm::vec3 m_color;         // 颜色
    glm::vec3 m_color_end;      // 结束颜色（用于渐变）
    float m_size;              // 大小
    float m_size_end;           // 结束大小
    float m_life;              // 当前生命
    float m_max_life;           // 最大生命
    float m_age;               // 已存活时间
    
    bool IsAlive() const { return m_life > 0.0f; }
    float GetLifeRatio() const { return m_life / m_max_life; }
};

#endif
