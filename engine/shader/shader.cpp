#include <glad/gl.h> //  必须在所有库的顶部

#include "shader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

Shader::Shader()
{
    m_program = glCreateProgram();
}

Shader::~Shader()
{
}

Shader::Shader(const char *vertexShaderPath, const char *fragmentShaderPath)
{
    m_program = glCreateProgram();
    addShaderFromSourceFile(VERTEX_SHADER, vertexShaderPath);
    addShaderFromSourceFile(FRAGMENT_SHADER, fragmentShaderPath);
}


void Shader::addShaderFromSourceFile(ShaderType shaderType, const char *filePath)
{
    std::cout << "add shader from source file: " << shaderType << std::endl;
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
    string shader_source = buffer.str();
    const GLchar *vertexShaderSource = shader_source.c_str();

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

bool Shader::Link()
{
    int success;

    glLinkProgram(m_program);

    // check for linking errors
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        PrintProgramLog(m_program);
    }
    return success == 1;
}

void Shader::BindFragDataLocation()
{
    glBindFragDataLocation(m_program, 0, "color\x00");
}

// 捕获链接着色器时的错误的函数
void Shader::PrintProgramLog(unsigned int id)
{
    int len = 0;
    char *log;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
    if (len > 0)
    {
        log = (char *)malloc(len);
        glGetProgramInfoLog(id, len, 0, log);
        std::cout << "Program Inof Log:" << log << std::endl;
        free(log);
    }
}


unsigned int Shader::GetAttributeLocation(const char *name)
{

    if (m_program != 0)
    {
        return glGetAttribLocation(m_program, name);
    }
    return 0;
}

unsigned int Shader::GetUniformLocation(const char *name)
{
    if (m_program != 0)
    {
        return glGetUniformLocation(m_program, name);
    }
    return 0;
}

void Shader::Use()
{
    glUseProgram(m_program);
}

bool Shader::UnUse()
{
    glUseProgram(0);
    return true;
}


void Shader::SetUniformValue(const char *name, float value)
{
}

void Shader::SetUniformValue(const char *name, int value)
{
}

void Shader::SetUniformValue(const char *name, bool value)
{
}

void Shader::SetUniformValue(const char *name, const glm::vec2 &value)
{
}

void Shader::SetUniformValue(const char *name, const glm::mat4 &value)
{
    glUniformMatrix4fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetUniformValue(unsigned int uniform_location, float value)
{
    glUniform1f(uniform_location, value);
}

void Shader::SetUniformValue(unsigned int uniform_location, int value)
{
    glUniform1i(uniform_location, value);
}

void Shader::SetUniformValue(unsigned int uniform_location, bool value)
{
}

void Shader::SetUniformValue(unsigned int uniform_location, const glm::vec2 &value)
{
    glUniform2fv(uniform_location, 1, glm::value_ptr(value));
}

void Shader::SetUniformValue(unsigned int uniform_location, const glm::vec3 &value)
{
    glUniform3fv(uniform_location, 1, glm::value_ptr(value));
}

void Shader::SetUniformValue(unsigned int uniform_location, const glm::mat4 &value)
{
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, glm::value_ptr(value));
}
