#include <glad/gl.h> //  必须在所有库的顶部

#include "shader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader()
{
}

Shader::~Shader()
{
}

void Shader::init()
{
    // glad 初始化
    if (!gladLoaderLoadGL())
    {
        std::cout << "gladLoadGLLoader error!" << std::endl;
    }
    m_program = glCreateProgram();
}

void Shader::addShaderFromSourceFile(ShaderType shaderType, const char *filePath)
{
    std::ifstream file(filePath);
    std::stringstream buffer;
    if (file.is_open())
    {
        buffer << file.rdbuf();
        file.close();
    }
    else
    {
        std::cerr << "无法打开文件进行读取: " << filePath << std::endl;
        return;
    }

    const GLchar *vertexShaderSource = buffer.str().c_str();

    int glShaderType = 0;
    if (shaderType == VERTEX_SHADER)
    {
        glShaderType = GL_VERTEX_SHADER;
    }
    else
    {
        glShaderType = GL_FRAGMENT_SHADER;
    }

    unsigned int vertexShader = glCreateShader(glShaderType);
    glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, 0, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    glAttachShader(m_program, vertexShader);

    glDeleteShader(vertexShader);
}

void Shader::link()
{
    int success;
    char infoLog[512];

    glLinkProgram(m_program);

    // check for linking errors
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(m_program, 512, 0, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
}

void Shader::use()
{
    glUseProgram(m_program);
}

void Shader::setUniformValue(const char *name, float value)
{
}

void Shader::setUniformValue(const char *name, int value)
{
}

void Shader::setUniformValue(const char *name, bool value)
{
}

void Shader::setUniformValue(const char *name, glm::vec2 value)
{
}

void Shader::setUniformValue(const char *name, glm::mat4 value)
{
    glUniformMatrix4fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(value));
}
