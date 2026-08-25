#ifndef __PARTICLE_H__
#define __PARTICLE_H__

#include <glm/glm.hpp>

struct Particle {
    glm::vec3 Position;      // 位置
    glm::vec3 Velocity;      // 速度
    glm::vec3 Color;         // 颜色
    glm::vec3 ColorEnd;      // 结束颜色（用于渐变）
    float Size;              // 大小
    float SizeEnd;           // 结束大小
    float Life;              // 当前生命
    float MaxLife;           // 最大生命
    float Age;               // 已存活时间
    
    bool IsAlive() const { return Life > 0.0f; }
    float GetLifeRatio() const { return Life / MaxLife; }
};

#endif
