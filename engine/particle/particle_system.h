#ifndef __PARTICLE_SYSTEM_H__
#define __PARTICLE_SYSTEM_H__

#include <glm/glm.hpp>
#include <vector>
#include <string>

class ParticleEmitter;
class Technique;
class Light;
struct RenderContext;

// 粒子顶点数据结构(上传到GPU的布局)
struct ParticleVertex {
    glm::vec3 m_position;
    glm::vec3 m_color;
    float m_size;
    float m_life;
    float m_max_life;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();
    ParticleSystem(const ParticleSystem &) = delete;
    ParticleSystem &operator=(const ParticleSystem &) = delete;
    
    void Init(const glm::vec3& position);
    void Update(float deltaTime);
    void Draw(const RenderContext &ctx, const glm::mat4& model);
    
    ParticleEmitter* GetEmitter() const { return m_emitter; }
    void ReallocateVBO();
    
private:
    ParticleEmitter* m_emitter;
    Technique* m_effect;
    unsigned int m_texture_id;
    
    // OpenGL对象
    unsigned int m_vao;
    unsigned int m_vbo;
    int m_vertex_count;
    
    // 复用的顶点缓冲，避免每帧重新分配
    std::vector<ParticleVertex> m_vertices;
    
    void UpdateBuffers();
};

#endif
