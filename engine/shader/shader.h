#ifndef __SHADER_H__
#define __SHADER_H__

#include <glm/glm.hpp>

enum ShaderType
{
    VERTEX_SHADER,
    FRAGMENT_SHADER
};

class Shader
{
public:
    Shader();
    ~Shader();

    void init();

    void addShaderFromSourceFile(ShaderType shaderType, const char *filePath);
    void load(const char *vertexShaderPath, const char *fragmentShaderPath);
    void use();

    void link();

    void setUniformValue(const char *name, float value);
    void setUniformValue(const char *name, int value);
    void setUniformValue(const char *name, bool value);
    void setUniformValue(const char *name, glm::vec2 value);
    void setUniformValue(const char *name, glm::mat4 value);

private:
    unsigned int m_program;
};

#endif