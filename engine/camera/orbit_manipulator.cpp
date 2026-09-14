#include "orbit_manipulator.h"
#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>

/*
 * OrbitManipulator 实现
 *
 * 轨道运动的数学基础（与旧 Camera::OrbitAroundOrigin 相同，但状态独立）：
 *   - 相机位置 = 中心 + 方向(dir) × 半径，方向由水平角 yaw 与俯仰角 pitch 决定：
 *       dir.x = cos(pitch)*cos(yaw)
 *       dir.y = sin(pitch)
 *       dir.z = cos(pitch)*sin(yaw)
 *   - 距中心过近时直接跳过，防止除零与坐标漂移
 *   - 每次姿态参数（yaw/pitch/半径/中心）变更后调用 ApplyToCamera()
 *     把结果写回所绑定的 Camera（位置 + 四元数朝向），Camera 只据此生成视图矩阵
 */

OrbitManipulator::OrbitManipulator() = default;

// 以相机当前状态反推轨道参数（初始绑定 / 相机切换后同步用）
void OrbitManipulator::ResetFromCamera() {
    if (m_camera == nullptr) {
        return;
    }

    // 轨道中心 = 相机位置 + 视向 × 参考距离（m_radius 保持当前缩放级别）。
    // 视向由相机朝向四元数推导；这样切换相机时轨道中心自然落在新相机
    // 正前方，相机保持原位不动，不会因沿用旧的轨道中心产生视角跳变。
    const glm::vec3 front = m_camera->GetOrientation() * glm::vec3(0.0f, 0.0f, -1.0f);
    m_center = m_camera->GetPosition() + front * m_radius;

    // 相机相对轨道中心的偏移向量（= -front × radius），反解球坐标
    const glm::vec3 offset = m_camera->GetPosition() - m_center;
    m_radius = glm::length(offset);
    // 相机恰好位于中心时无法解出角度（除零），保持当前角度不变
    if (m_radius < 0.001f) {
        return;
    }
    // yaw = atan2(z, x)，pitch = asin(y / r)，换算为角度制便于面板显示/编辑
    m_yaw = glm::degrees(std::atan2(offset.z, offset.x));
    m_pitch = glm::degrees(std::asin(offset.y / m_radius));
    ClampPitch();
}

/*
 * 把当前球坐标参数写回绑定的 Camera
 *
 * 位置 = 中心 + 方向×半径；朝向用四元数表示（glm::quatLookAt 由
 * "相机看向的方向 + 世界上方向"直接构造），替代旧实现每次重新计算
 * front/right/up 三向量的方式，旋转表达更紧凑且无万向节锁。
 */
void OrbitManipulator::ApplyToCamera() {
    if (m_camera == nullptr) {
        return;
    }
    // 相机始终看向轨道中心：朝向 = 中心 - 位置 = -方向
    const glm::vec3 dir = ComputeDirection();
    const glm::vec3 position = m_center + dir * m_radius;
    const glm::quat orientation = glm::quatLookAt(-dir, glm::vec3(0.0f, 1.0f, 0.0f));

    m_camera->SetPosition(position);
    m_camera->SetOrientation(orientation);
}

// ---- 交互操作：更新轨道参数并立即落回相机 ----

void OrbitManipulator::Orbit(float horizontalAngle, float verticalAngle) {
    m_yaw += horizontalAngle;
    m_pitch += verticalAngle;
    ClampPitch();
    ApplyToCamera();
}

void OrbitManipulator::Pan(float dx, float dy) {
    // 沿当前视图的右向/上向平移轨道中心（相机随中心移动，姿态不变）
    const glm::vec3 dir = ComputeDirection();
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    // 右向 = dir × up 归一化，上向 = 右向 × dir（右手系正交基）
    const glm::vec3 right = glm::normalize(glm::cross(dir, up));
    const glm::vec3 camUp = glm::normalize(glm::cross(right, dir));

    m_center += right * dx;
    m_center += camUp * dy;
    ApplyToCamera();
}

void OrbitManipulator::Zoom(float amount) {
    // 靠近中心（正值）时半径减小，限定最小距离防止穿过中心
    m_radius -= amount;
    if (m_radius < 0.5f) {
        m_radius = 0.5f;
    }
    ApplyToCamera();
}

// ---- 轨道参数设定（ImGui 面板编辑入口，设定后同步相机姿态） ----

void OrbitManipulator::SetCenter(const glm::vec3 &center) {
    m_center = center;
    ApplyToCamera();
}

void OrbitManipulator::SetRadius(float radius) {
    m_radius = radius;
    if (m_radius < 0.5f) {
        m_radius = 0.5f;
    }
    ApplyToCamera();
}

void OrbitManipulator::SetYaw(float yaw) {
    m_yaw = yaw;
    ApplyToCamera();
}

void OrbitManipulator::SetPitch(float pitch) {
    m_pitch = pitch;
    ClampPitch();
    ApplyToCamera();
}

// ---- 内部工具 ----

glm::vec3 OrbitManipulator::ComputeDirection() const {
    const float yawRad = glm::radians(m_yaw);
    const float pitchRad = glm::radians(m_pitch);
    const float cp = std::cos(pitchRad);
    return glm::vec3(cp * std::cos(yawRad), std::sin(pitchRad), cp * std::sin(yawRad));
}

void OrbitManipulator::ClampPitch() {
    // 限制在 ±89° 附近，避免俯仰到极点时方向退化为竖直（万向节锁）
    constexpr float kPitchLimit = 89.0f;
    if (m_pitch > kPitchLimit) {
        m_pitch = kPitchLimit;
    }
    if (m_pitch < -kPitchLimit) {
        m_pitch = -kPitchLimit;
    }
}