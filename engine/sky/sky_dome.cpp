#include "sky_dome.h"
#include "../technique/technique.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SkyDome::SkyDome() {
}

SkyDome::~SkyDome() {
    if (m_effect) {
        delete m_effect;
        m_effect = nullptr;
    }
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
    }
    if (m_VBO) {
        glDeleteBuffers(1, &m_VBO);
    }
    if (m_EBO) {
        glDeleteBuffers(1, &m_EBO);
    }
}

void SkyDome::Init(float radius, int sectors, int stacks) {
    m_radius = radius;
    m_sectors = sectors;
    m_stacks = stacks;

    m_effect = new Technique(
        "sky",
        "./resource/shader/sky.vert",
        "./resource/shader/sky.frag"
    );

    GenerateHemisphere(radius, sectors, stacks);
}

/*
 * 生成半球体网格
 *
 * 使用经纬度参数化生成半球体（从赤道到天顶）。
 * 顶点格式：position (vec3) + normal (vec3)
 * 法线方向与位置相同（归一化后即为方向向量），用于计算渐变。
 *
 * 球面参数方程：
 *   x = r * cos(latitude) * cos(longitude)
 *   y = r * sin(latitude)                    // 纬度从 0（赤道）到 PI/2（天顶）
 *   z = r * cos(latitude) * sin(longitude)
 */
void SkyDome::GenerateHemisphere(float radius, int sectors, int stacks) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // 生成顶点
    // 纬度从 0（赤道）到 PI/2（天顶），共 stacks+1 行
    for (int stack = 0; stack <= stacks; ++stack) {
        // 纬度角：0 = 赤道，PI/2 = 天顶
        float latitude = (static_cast<float>(stack) / stacks) * static_cast<float>(M_PI) * 0.5f;

        // 经度从 0 到 2*PI，共 sectors+1 列（首尾相连）
        for (int sector = 0; sector <= sectors; ++sector) {
            // 经度角：0 到 2*PI
            float longitude = (static_cast<float>(sector) / sectors) * 2.0f * static_cast<float>(M_PI);

            // 球面坐标转笛卡尔坐标
            float x = radius * cosf(latitude) * cosf(longitude);
            float y = radius * sinf(latitude);
            float z = radius * cosf(latitude) * sinf(longitude);

            // 位置
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // 法线（归一化后即为方向向量，用于渐变计算）
            float nx = cosf(latitude) * cosf(longitude);
            float ny = sinf(latitude);
            float nz = cosf(latitude) * sinf(longitude);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }

    // 生成三角形索引
    // 每个网格单元由两个三角形组成
    for (int stack = 0; stack < stacks; ++stack) {
        for (int sector = 0; sector < sectors; ++sector) {
            // 当前行和下一行的顶点索引
            int current = stack * (sectors + 1) + sector;
            int next = current + sectors + 1;

            // 第一个三角形
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            // 第二个三角形
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    m_indexCount = static_cast<unsigned int>(indices.size());

    // 创建 OpenGL 缓冲区对象
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    // 上传顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // 上传索引数据
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    // 顶点属性 0：位置 (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void *) 0);

    // 顶点属性 1：法线 (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void *) (3 * sizeof(float)));

    glBindVertexArray(0);
}

void SkyDome::Draw(long long elapsed,
                   const glm::mat4 &projection,
                   const glm::mat4 &view,
                   const glm::vec3 &cameraPos) {
    if (!m_effect || m_indexCount == 0) return;

    // 激活着色器
    m_effect->Enable();

    // 设置变换矩阵
    m_effect->SetProjectionMatrix(projection);
    m_effect->SetViewMatrix(view);

    // 模型矩阵：仅平移到摄像机位置（天空穹始终以摄像机为中心）
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, cameraPos);
    m_effect->SetModelMatrix(model);

    // 设置渐变颜色
    m_effect->SetUniform("horizonColor", m_horizonColor);
    m_effect->SetUniform("zenithColor", m_zenithColor);

    // 临时切换深度函数为 GL_LEQUAL，允许天空穹在远裁剪面（depth=1.0）通过深度测试
    glDepthFunc(GL_LEQUAL);

    // 绘制半球体
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // 恢复默认深度函数
    glDepthFunc(GL_LESS);
}

void SkyDome::SetHorizonColor(const glm::vec3 &color) {
    m_horizonColor = color;
}

void SkyDome::SetZenithColor(const glm::vec3 &color) {
    m_zenithColor = color;
}
