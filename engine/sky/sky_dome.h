#ifndef __SKY_DOME_H__
#define __SKY_DOME_H__

#include <glm/glm.hpp>

class Technique;

/*
 * 天空穹（Sky Dome）
 *
 * 渲染一个以摄像机为中心的半球体，模拟天空背景。
 * 使用从地平线到天顶的渐变颜色，营造真实天空效果。
 *
 * 渲染要点：
 *   - 必须在所有场景物体之前绘制
 *   - 禁用深度写入（glDepthMask(GL_FALSE)），保持深度测试
 *   - 模型矩阵仅包含摄像机位置平移（天空穹始终跟随摄像机）
 *   - 顶点着色器将 z 强制设为 w，使深度 = 1.0（远裁剪面）
 */
class SkyDome {
public:
    SkyDome();
    ~SkyDome();

    /*
     * 初始化天空穹
     * radius: 半球半径（必须大于裁剪面远距离）
     * sectors: 水平方向分段数（经度方向）
     * stacks: 垂直方向分段数（纬度方向，从赤道到天顶）
     */
    void Init(float radius, int sectors, int stacks);

    /*
     * 绘制天空穹
     * elapsed: 经过时间（毫秒）
     * projection: 投影矩阵
     * view: 视图矩阵
     * cameraPos: 摄像机世界位置（用作天空穹中心）
     */
    void Draw(long long elapsed,
              const glm::mat4 &projection,
              const glm::mat4 &view,
              const glm::vec3 &cameraPos);

    /* 设置地平线颜色（半球底部） */
    void SetHorizonColor(const glm::vec3 &color);

    /* 设置天顶颜色（半球顶部） */
    void SetZenithColor(const glm::vec3 &color);

private:
    /* 半球体网格数据 */
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
    unsigned int m_indexCount = 0;

    /* 着色器技术 */
    Technique *m_effect = nullptr;

    /* 渐变颜色 */
    glm::vec3 m_horizonColor = glm::vec3(0.6f, 0.7f, 0.9f);
    glm::vec3 m_zenithColor = glm::vec3(0.1f, 0.2f, 0.5f);

    /* 生成半球体网格 */
    void GenerateHemisphere(float radius, int sectors, int stacks);
};

#endif // __SKY_DOME_H__
