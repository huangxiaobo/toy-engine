#include <glad/gl.h> //  必须在所有库的顶部

#include "shader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

/*
 * Shader 封装类：负责 GLSL 着色器的创建、编译、链接与 uniform 设置
 *
 * 职责：
 *   1. 从文件加载着色器源码并编译（顶点/片段）
 *   2. 链接为 GPU program
 *   3. 提供 uniform/attribute 定位与值设置的重载接口
 *
 * 使用约定：着色器源码字符串必须以 '\0' 结尾，否则 GLSL 编译报 syntax error。
 */

Shader::Shader() {
    m_program = glCreateProgram();
}

Shader::~Shader() {
    // 依赖 Cleanup() 中 m_renderer.reset() 早于 glfwTerminate，保证此处 GL 上下文仍有效
    if (m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

/*
 * 带源码路径的构造函数
 *
 * 一步完成：创建 program -> 编译顶点着色器 -> 编译片段着色器。
 * 注意：编译后还需调用 Link() 完成链接（Technique 构造流程中调用）。
 */
Shader::Shader(const char *vertexShaderPath, const char *fragmentShaderPath) {
    m_program = glCreateProgram();
    addShaderFromSourceFile(VERTEX_SHADER, vertexShaderPath);
    addShaderFromSourceFile(FRAGMENT_SHADER, fragmentShaderPath);
}

/*
 * 从文件编译单个着色器阶段并挂载到 program
 *
 * 参数：
 *   shaderType - VERTEX_SHADER 或 FRAGMENT_SHADER
 *   filePath   - .vert / .frag 文件路径
 *
 * 流程：读文件 -> 创建 shader 对象 -> 上传源码 -> 编译 -> 检查错误 -> 挂载 -> 释放 shader 对象。
 * 编译失败时仅打印日志并继续（不中断），最终链接由 Link() 验证。
 * 着色器对象在挂载后即可删除，program 中已保留一份副本。
 */
void Shader::addShaderFromSourceFile(ShaderType shaderType, const char *filePath) {
    std::cout << "add shader from source file: " << shaderType << std::endl;
    std::ifstream file(filePath);
    std::stringstream buffer;
    if (file.is_open()) {
        buffer << file.rdbuf();
        file.close();
    } else {
        std::cerr << "无法打开文件进行读取: " << filePath << std::endl;
        return;
    }
    string shader_source = buffer.str();
    const GLchar *vertexShaderSource = shader_source.c_str();

    int glShaderType = 0;
    if (shaderType == VERTEX_SHADER) {
        glShaderType = GL_VERTEX_SHADER;
    } else {
        glShaderType = GL_FRAGMENT_SHADER;
    }

    unsigned int vertexShader = glCreateShader(glShaderType);
    glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, 0, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                << infoLog << std::endl;
    }

    glAttachShader(m_program, vertexShader);
    glDeleteShader(vertexShader);
}

/*
 * 链接 program
 *
 * 链接成功后才能使用 uniform 和绘制。失败时打印完整日志并返回 false。
 * Technique 构造函数在链接失败时会 exit(-1) 直接终止程序。
 */
bool Shader::Link() {
    int success;

    // uniform 位置在链接完成前无效，这里清一次缓存；
    // 顺带覆盖同一个 Shader 重复链接的情况，避免沿用上一次的旧位置
    m_uniform_locations.clear();

    // 绑定输出到 location 0 必须在链接之前声明才生效（链接期绑定）。
    // 原实现把 BindFragDataLocation 放在 Technique::Enable() 每帧调用，
    // 但 glBindFragDataLocation 只在 glLinkProgram 时生效，运行期重复调用是纯开销。
    glBindFragDataLocation(m_program, 0, "color\x00");

    glLinkProgram(m_program);

    // check for linking errors
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        PrintProgramLog(m_program);
    }
    return success == 1;
}

/*
 * 捕获链接着色器时的错误的函数
 */
void Shader::PrintProgramLog(unsigned int id) {
    int len = 0;
    char *log;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        log = (char *) malloc(len);
        glGetProgramInfoLog(id, len, 0, log);
        std::cout << "Program Inof Log:" << log << std::endl;
        free(log);
    }
}


unsigned int Shader::GetAttributeLocation(const char *name) {
    if (m_program != 0) {
        return glGetAttribLocation(m_program, name);
    }
    return 0;
}

// 按名称查询 uniform 位置；未找到时返回 -1（GL 规范），调用方按 0 处理沿用现状
unsigned int Shader::GetUniformLocation(const char *name) {
    if (m_program != 0) {
        return glGetUniformLocation(m_program, name);
    }
    return 0;
}

// 激活该 program 作为当前绘制上下文
void Shader::Use() {
    glUseProgram(m_program);
}

bool Shader::UnUse() {
    glUseProgram(0);
    return true;
}

/*
 * 按名取 uniform 位置，带缓存
 *
 * 第一次遇到某个名称时才真正向驱动查询，之后直接命中 m_uniform_locations。
 * 绘制热路径（如 Mesh::Draw 每次设置 gUseShadow/gHasTexture 等）每帧会调用多次，
 * 不缓存就等于每帧重复做一遍驱动内部的 uniform 表字符串查找。
 *
 * 关于取不到的情况：glGetUniformLocation 找不到时返回 -1，存进 unsigned int 会变成
 * 全 1 的位模式，glUniform* 收到后按 GL 规范静默忽略。这里缓存的正是这个值，
 * 所以未声明的 uniform 依旧被静默跳过，与加缓存之前的行为完全一致。
 */
unsigned int Shader::ResolveUniformLocation(const char *name) {
    auto found = m_uniform_locations.find(name);
    if (found != m_uniform_locations.end()) {
        return found->second;
    }
    unsigned int location = static_cast<unsigned int>(glGetUniformLocation(m_program, name));
    m_uniform_locations.emplace(name, location);
    return location;
}

// ---- 按名称设置 uniform 的重载族（位置走缓存，见 ResolveUniformLocation） ----

void Shader::SetUniformValue(const char *name, float value) {
    glUniform1f(ResolveUniformLocation(name), value);
}

void Shader::SetUniformValue(const char *name, int value) {
    glUniform1i(ResolveUniformLocation(name), value);
}

// bool 重载当前为空实现（预留）
void Shader::SetUniformValue(const char *name, bool value) {
}

void Shader::SetUniformValue(const char *name, const glm::vec2 &value) {
    glUniform2fv(ResolveUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetUniformValue(const char *name, const glm::vec3 &value) {
    SetUniformValue(ResolveUniformLocation(name), value);
}

void Shader::SetUniformValue(const char *name, const glm::vec4 &value) {
    glUniform4fv(ResolveUniformLocation(name), 1, glm::value_ptr(value));
}

// 矩阵按列主序上传（GL_FALSE = 不转置），与 glm 默认内存布局一致
void Shader::SetUniformValue(const char *name, const glm::mat4 &value) {
    glUniformMatrix4fv(ResolveUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

// ---- 按缓存 location 设置 uniform 的重载族（推荐：location 预取一次，避免逐帧字符串查询） ----

void Shader::SetUniformValue(unsigned int uniform_location, float value) {
    glUniform1f(uniform_location, value);
}

void Shader::SetUniformValue(unsigned int uniform_location, int value) {
    glUniform1i(uniform_location, value);
}

// bool 重载当前为空实现（预留）
void Shader::SetUniformValue(unsigned int uniform_location, bool value) {
}

void Shader::SetUniformValue(unsigned int uniform_location, const glm::vec2 &value) {
    glUniform2fv(uniform_location, 1, glm::value_ptr(value));
}

void Shader::SetUniformValue(unsigned int uniform_location, const glm::vec3 &value) {
    glUniform3fv(uniform_location, 1, glm::value_ptr(value));
}

void Shader::SetUniformValue(unsigned int uniform_location, const glm::mat4 &value) {
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, glm::value_ptr(value));
}