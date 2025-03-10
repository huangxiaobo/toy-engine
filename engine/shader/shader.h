#ifndef __SHADER_H__
#define __SHADER_H__

#include <glm/glm.hpp>

enum ShaderType
{
    VERTEX_SHADER,
    FRAGMENT_SHADER,
};

class Shader
{
public:
    Shader();

    Shader(const char *vertexShaderPath, const char *fragmentShaderPath);
    ~Shader();
    void addShaderFromSourceFile(ShaderType shaderType, const char *filePath);
    void Use();
    bool UnUse();
    bool Link();

    unsigned int GetAttributeLocation(const char *name);

    unsigned int GetUniformLocation(const char *name);

    void SetUniformValue(const char *name, float value);
    void SetUniformValue(const char *name, int value);
    void SetUniformValue(const char *name, bool value);
    void SetUniformValue(const char *name, const glm::vec2 &value);
    void SetUniformValue(const char *name, const glm::vec3 &value);
    void SetUniformValue(const char *name, const glm::mat4 &value);

    void SetUniformValue(unsigned int uniform_location, float value);
    void SetUniformValue(unsigned int uniform_location, int value);
    void SetUniformValue(unsigned int uniform_location, bool value);
    void SetUniformValue(unsigned int uniform_location, const glm::vec2 &value);
    void SetUniformValue(unsigned int uniform_location, const glm::vec3 &value);
    void SetUniformValue(unsigned int uniform_location, const glm::mat4 &value);

    void BindColorAttribute(unsigned int index, const char *name);

    void BindFragDataLocation();

private:
    void PrintProgramLog(unsigned int id);

private:
    unsigned int m_program;
};

#endif