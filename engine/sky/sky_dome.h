#ifndef __SKY_DOME_H__
#define __SKY_DOME_H__

#include <glm/glm.hpp>

class Technique;
struct RenderContext;

/*
 * 天空穹（Sky Dome）
 *
 * 渲染一个以摄像机为中心的全景球体，模拟天空背景。
 * 上半球使用从地平线到天顶的渐变颜色；下半球从地平线平滑过渡到
 * 地面雾色（避开露底色，也让正侧/俯视等视角地平线以下更自然）。
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
     * radius: 全球体半径（需大到覆盖视锥体内所有方向，见 sky_dome.cpp 说明）
     * sectors: 水平方向分段数（经度方向）
     * stacks: 垂直方向分段数（纬度方向，从南极到北极）
     */
    void Init(float radius, int sectors, int stacks);

    /*
     * 绘制天空穹
     * ctx: 帧级渲染上下文（投影/视图/相机位置，见 render_context.h）
     */
    void Draw(const RenderContext &ctx);

    /* 设置地平线颜色（上半球底部） */
    void SetHorizonColor(const glm::vec3 &color);

    /* 设置天顶颜色（上半球顶部） */
    void SetZenithColor(const glm::vec3 &color);

    /* 设置地面雾色（下半球底部方向），地平线以下平滑过渡到该颜色 */
    void SetGroundColor(const glm::vec3 &color);

    /* 获取天空穹属性（供 ImGui 属性面板使用） */
    float GetRadius() const { return m_radius; }
    int GetSectors() const { return m_sectors; }
    int GetStacks() const { return m_stacks; }
    glm::vec3 GetHorizonColor() const { return m_horizonColor; }
    glm::vec3 GetZenithColor() const { return m_zenithColor; }
    glm::vec3 GetGroundColor() const { return m_groundColor; }

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
    // 地面雾色（下半球），默认取地平线色的暗化版本（0.6 倍）
    glm::vec3 m_groundColor = glm::vec3(0.36f, 0.42f, 0.54f);

    /* 生成全球体网格 */
    void GenerateSphere(float radius, int sectors, int stacks);

    /* 缓存初始化参数，供 getter 使用 */
    float m_radius = 0.0f;
    int m_sectors = 0;
    int m_stacks = 0;
};

#endif // __SKY_DOME_H__
