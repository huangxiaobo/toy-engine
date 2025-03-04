#ifndef __SHADER_H__
#define __SHADER_H__

#include <glm/glm.hpp>
class Shader
{
public:
    Shader();
    ~Shader();

    void init();
    void load(const char *vertexShaderPath, const char *fragmentShaderPath);
    void use();
    void setUniform(const char *name, float value);
    void setUniform(const char *name, int value);
    void setUniform(const char *name, bool value);
    void setUniform(const char *name, glm::vec2 value);

private:
    unsigned int m_program;
    unsigned int m_vert_shader;
    unsigned int m_frag_shader;
};

#endif