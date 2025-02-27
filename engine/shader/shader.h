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

    bool unbind();
    bool bind();

    bool link();

    unsigned int attributeLocation(const char *name);

    unsigned int uniformLocation(const char *name);

    void setUniformValue(const char *name, float value);
    void setUniformValue(const char *name, int value);
    void setUniformValue(const char *name, bool value);
    void setUniformValue(const char *name, const glm::vec2 &value);
    void setUniformValue(const char *name, const glm::vec3 &value);
    void setUniformValue(const char *name, const glm::mat4 &value);

    void setUniformValue(unsigned int uniform_location, float value);
    void setUniformValue(unsigned int uniform_location, int value);
    void setUniformValue(unsigned int uniform_location, bool value);
    void setUniformValue(unsigned int uniform_location, const glm::vec2 &value);
    void setUniformValue(unsigned int uniform_location, const glm::vec3 &value);
    void setUniformValue(unsigned int uniform_location, const glm::mat4 &value);

    void BindColorAttribute(unsigned int index, const char *name);

    void BindFragDataLocation();

private:
    void PrintProgramLog(unsigned int id);

private:
    unsigned int m_program;
};

#endif