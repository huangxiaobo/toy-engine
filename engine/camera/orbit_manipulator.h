#ifndef __ORBIT_MANIPULATOR_H__
#define __ORBIT_MANIPULATOR_H__

#include "manipulator.h"

#include <glm/glm.hpp>

/*
 * 轨道相机操控器（OrbitManipulator）
 *
 * 相机绕着轨道中心点（默认为世界原点）旋转，用户交互与以前的
 * Camera::OrbitAroundOrigin/Pan/Zoom 语义一致，但状态独立于 Camera：
 *
 *   内部参数（球坐标）：
 *     - m_center  ：轨道中心（即相机始终看向的点，旧实现固定为原点）
 *     - m_radius  ：相机到中心的距离
 *     - m_yaw     ：绕世界 Y 轴的水平角（度）
 *     - m_pitch   ：绕相机右向的俯仰角（度，限制在 ±89° 防万向节锁）
 *
 * 每次 Orbit/Pan/Zoom 之后调用 ApplyToCamera() 把最新姿态写回所绑定的
 * Camera（位置 + 四元数朝向），Camera 只负责根据姿态生成视图矩阵。
 */
class OrbitManipulator : public CameraManipulator {
public:
    OrbitManipulator();

    // 以相机当前状态反推轨道参数（初始绑定、相机被外部 SetPosition 后同步用）
    void ResetFromCamera();

    // 绕中心水平/垂直旋转
    void Orbit(float horizontalAngle, float verticalAngle);
    // 平移：相机与轨道中心整体沿右向(x)/上向(y)移动，姿态不变
    void Pan(float dx, float dy);
    // 缩放：改变半径（推远/拉近），正值放大（靠近中心）
    void Zoom(float amount);

    // ---- 轨道参数访问（供 ImGui 属性面板编辑/显示） ----
    glm::vec3 GetCenter() const { return m_center; }
    float GetRadius() const { return m_radius; }
    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }

    void SetCenter(const glm::vec3 &center); // 设定后同步相机姿态
    void SetRadius(float radius);
    void SetYaw(float yaw);
    void SetPitch(float pitch);

    void ApplyToCamera() override;

private:
    // 以当前 yaw/pitch 计算"相机看向中心"的单位方向向量（球坐标 → 笛卡尔）
    glm::vec3 ComputeDirection() const;
    // 把 pitch 限制在 ±89°，避免万向节锁
    void ClampPitch();

private:
    glm::vec3 m_center = glm::vec3(0.0f); // 轨道中心
    float m_radius = 30.0f;               // 相机与中心距离
    float m_yaw = 0.0f;                   // 水平角（度）
    float m_pitch = 0.0f;                 // 俯仰角（度）
};

#endif // __ORBIT_MANIPULATOR_H__