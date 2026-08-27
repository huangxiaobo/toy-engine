#ifndef __PARTICLE_SYSTEM_H__
#define __PARTICLE_SYSTEM_H__

#include <glm/glm.hpp>
#include <vector>
#include <string>

class ParticleEmitter;
class Technique;
class Light;

// 粒子顶点数据结构(上传到GPU的布局)
struct ParticleVertex {
    glm::vec3 Position;
    glm::vec3 Color;
    float Size;
    float Life;
    float MaxLife;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();
    
    void Init(const glm::vec3& position);
    void Update(float deltaTime);
    void Draw(long long elapsed,
              const glm::mat4& projection,
              const glm::mat4& view,
              const glm::mat4& model,
              const glm::vec3& camera,
              const std::vector<Light*>& lights);
    
    ParticleEmitter* GetEmitter() const { return m_emitter; }
    void ReallocateVBO();
    
private:
    ParticleEmitter* m_emitter;
    Technique* m_effect;
    unsigned int m_textureID;
    
    // OpenGL对象
    unsigned int m_VAO;
    unsigned int m_VBO;
    int m_vertexCount;
    
    // 复用的顶点缓冲，避免每帧重新分配
    std::vector<ParticleVertex> m_vertices;
    
    void UpdateBuffers();
};

#endif
