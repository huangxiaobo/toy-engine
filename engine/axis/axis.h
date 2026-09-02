#ifndef __AXIS_H__
#define __AXIS_H__

#include <glm/glm.hpp>

struct ImVec2;

/*
 * 屏幕空间坐标轴 gizmo（Axis）
 *
 * 参照 erhe 引擎（ImViewGuizmo，Marcel Kazemi 作品）在视口角落叠加的
 * 世界坐标系指示器实现：
 *   - 屏幕空间叠加绘制，不参与 3D 场景渲染（非场景网格对象）
 *   - 使用相机视图矩阵的旋转部分（去掉平移），将世界轴方向投影到屏幕
 *   - 六轴 ±X/±Y/±Z 按视图深度排序，背向相机的轴不绘制
 *   - 三轴配色：X 红 / Y 绿 / Z 蓝，透明度随深度衰减
 *   - 每轴末端绘制实心圆点把手，并在把手处绘制 X/Y/Z 文字标签
 * 通过 ImGui::GetBackgroundDrawList() 叠加到 3D 场景之上。
 */
class Axis
{
public:
    Axis();
    ~Axis();

    // 设置整体缩放比例（默认 0.6f，决定 gizmo 在屏幕上的大小）
    void SetScale(float scale);
    float GetScale() const { return m_scale; }

    // 在视口角落绘制坐标系 gizmo
    // viewMatrix: 相机视图矩阵（仅使用其旋转部分，平移会被忽略）
    // center:     gizmo 中心在视口（ImGui 逻辑坐标）中的位置
    void Draw(const glm::mat4& viewMatrix, const ImVec2& center);

private:
    float m_scale = 0.6f;
};

#endif