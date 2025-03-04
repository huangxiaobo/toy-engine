#include <glad/gl.h> //  必须在所有库的顶部

#include "shader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <QDebug>
// #include <QOpenGLFunctions>
// #include <QOpenGLShaderProgram>

Shader::Shader()
{
}

Shader::~Shader()
{
}

void Shader::init()
{
}

void Shader::load(const char *vertexShaderPath, const char *fragmentShaderPath)
{
    std::cout << "1111" << std::endl;
    // glad 初始化
    if (!gladLoaderLoadGL())
    {
        std::cout << "gladLoadGLLoader error!" << std::endl;
        qDebug() << "gladLoadGLLoader error!";
    }
    std::cout << "2222" << std::endl;
    std::cout << glGetString(GL_VERSION);
    std::cout << "3333" << std::endl;

    std::ifstream file(vertexShaderPath);
    std::stringstream buffer;
    if (file.is_open())
    {
        buffer << file.rdbuf();
        file.close();
    }
    else
    {
        std::cerr << "无法打开文件进行读取: " << vertexShaderPath << std::endl;
    }

    std::cerr << "1111" << std::endl;

    const GLchar *vertexShaderSource = buffer.str().c_str();
    std::cerr << "1111" << std::endl;
    std::cerr << buffer.str() << std::endl;
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::cerr << "1112" << std::endl;
    glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
    std::cerr << "1111" << std::endl;
    glCompileShader(vertexShader);
    std::cerr << "1111" << std::endl;
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

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::ifstream file1(fragmentShaderPath);
    std::stringstream buffer1;
    if (file1.is_open())
    {
        buffer1 << file1.rdbuf();
        file1.close();
    }
    else
    {
        std::cerr << "无法打开文件进行读取: " << vertexShaderPath << std::endl;
    }
    const GLchar *fragmentShaderSource = buffer1.str().c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, 0);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, 0, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);
    // check for linking errors
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(m_program, 512, 0, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::use()
{
    glUseProgram(m_program);
}

void Shader::setUniform(const char *name, float value)
{
}

void Shader::setUniform(const char *name, int value)
{
}

void Shader::setUniform(const char *name, bool value)
{
}

void Shader::setUniform(const char *name, glm::vec2 value)
{
}
