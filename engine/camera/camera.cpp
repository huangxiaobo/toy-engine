#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

/* 默认构造函数：所有向量成员留空，需后续调用初始化接口 */
Camera::Camera()
{
}

/*
 * 参数化构造函数：以「位置-目标-世界上向量」初始化相机
 *
 * 根据 position 与 target 推导正交基：
 *   - m_front：从相机指向目标的方向（观察方向，取负即为视线方向）
 *   - m_right：front × world_up 归一化（世界右方向）
 *   - m_up   ：right × front 归一化（由前两者叉积得出，保证正交）
 * 移动灵敏度 0.1、缩放 45° 为默认值。
 */
Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 world_up)
    : m_move_speed(12.5f), m_mouse_sensitivity(0.1f), m_zoom(45.0f)
{
    m_position = position;
    m_target = target;
    m_world_up = world_up;

    m_front = glm::normalize(m_target - m_position);
    m_right = glm::normalize(glm::cross(m_front, m_world_up));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

/*
 * 获取视图矩阵（View Matrix）
 *
 * 使用 m_position（相机位置）、m_target（目标点）、m_world_up（世界上向量）
 * 构造 lookAt 矩阵。注意这里以 m_target 作为注视点而非 m_front，
 * 因此相机始终精确看向配置的目标位置（如原点附近场景中心）。
 */
glm::mat4 Camera::GetViewMatrix()
{
    // glm::LookAt函数需要一个相机世界空间位置、一个目标位置、世界空间中的上向量,创建作为观察矩阵
    // 使用m_target作为目标点，确保相机始终看向配置的目标位置
    return glm::lookAt(m_position, m_target, m_world_up);
}
/*
 * 处理键盘移动（第一人称位移）
 *
 * 注意：这里使用「位移 = 速度 × deltaTime」而非「速度 × deltaTime²」，
 * FORWARD/BACKWARD 沿 m_front 前后移动，LEFT/RIGHT 沿 front×world_up
 * 的侧向量左右移动，位移量与帧时间成正比（每秒 m_move_speed 单位的匀速移动）。
 */
void Camera::ProcessKeyboard(CameraMoveType direction, float deltaTime)
{
    const float velocity = m_move_speed * deltaTime;
    switch (direction)
    {
    case FORWARD:
        m_position += velocity * deltaTime;
        break;
    case BACKWARD:
        m_position -= velocity * deltaTime;
        break;
    case LEFT:
        m_position -= glm::normalize(glm::cross(m_front, m_world_up)) * velocity * deltaTime;
        break;
    case RIGHT:
        m_position += glm::normalize(glm::cross(m_front, m_world_up)) * velocity * deltaTime;
        break;
    default:
        break;
    }
}
/*
 * 处理鼠标滚轮缩放
 *
 * 滚轮上滑（yoffset>0）减小 m_zoom（视野更窄、画面放大）；
 * 缩放范围限定在 [1, 45] 度，防止透视畸变。
 */
void Camera::ProcessMouseScroll(float yoffset)
{
    if (m_zoom >= 1.0f && m_zoom <= 45.0f)
        m_zoom -= yoffset;
    if (m_zoom <= 1.0f)
        m_zoom = 1.0f;
    if (m_zoom >= 45.0f)
        m_zoom = 45.0f;
    this->updateCameraVectors();
}
/*
 * 更新相机的正交基向量（前/右）
 *
 * 该函数在相机方向被外部修改或位移后调用，用于重新计算
 * m_front（指向目标）与 m_right（front × world_up）。
 * 注意：此版本不重新计算 m_up（保持原始世界上方向结果），
 * 与构造函数中的完整正交化行为略有不同。
 */
void Camera::updateCameraVectors()
{
    // 更新相机的前向量和右向量
    m_front = glm::normalize(m_target - m_position);
    m_right = glm::normalize(glm::cross(m_front, m_world_up));
}
Camera::~Camera()
{
}

std::string Camera::GetName() const {
    return m_name;
}

// ... 已有代码 ...

/*
 * 绕相机前向向量旋转（翻滚 Roll）
 *
 * 以 m_front 为轴构造旋转矩阵，旋转相机的上向量和右向量。
 * 用于模拟飞机/第一人称视角的侧倾效果；m_front 本身不变。
 */
void Camera::Roll(float angle)
{
    // 创建一个绕相机前向向量旋转的旋转矩阵
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), m_front);
    
    // 使用旋转矩阵更新相机的上向量和右向量
    m_up = glm::vec3(rotation * glm::vec4(m_up, 0.0f));
    m_right = glm::vec3(rotation * glm::vec4(m_right, 0.0f));
}

/*
 * 绕世界 Y 轴水平旋转相机（类似轨道相机水平转动）
 *
 * 旋转矩阵作用于 m_front/m_right，并将注视点重置为世界原点，
 * 实现相机围绕场景中心做水平扫视（目标恒为原点）。
 */
void Camera::RotateHorizontal(float angle)
{
    // 绕世界Y轴旋转（水平旋转）
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), m_world_up);
    
    // 更新相机的前向向量和右向量
    m_front = glm::vec3(rotation * glm::vec4(m_front, 0.0f));
    m_right = glm::vec3(rotation * glm::vec4(m_right, 0.0f));
    
    // 更新目标点为原点（0,0,0），确保水平旋转时始终看向世界原点
    m_target = glm::vec3(0.0f, 0.0f, 0.0f);
}

/*
 * 绕相机右向量旋转（垂直旋转，类似俯仰）
 *
 * 旋转矩阵作用于 m_front/m_up，并将注视点重置为世界原点，
 * 实现相机在垂直方向的俯仰扫视（目标恒为原点）。
 */
void Camera::RotateVertical(float angle)
{
    // 绕相机右向量旋转（垂直旋转）
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), m_right);
    
    // 更新相机的前向向量和上向量
    m_front = glm::vec3(rotation * glm::vec4(m_front, 0.0f));
    m_up = glm::vec3(rotation * glm::vec4(m_up, 0.0f));
    
    // 更新目标点为原点（0,0,0），确保垂直旋转时始终看向世界原点
    m_target = glm::vec3(0.0f, 0.0f, 0.0f);
}

/*
 * 平移相机（Pan）
 *
 * 相机位置与注视点同时沿右向量（dx）和上向量（dy）移动，
 * 保持姿态不变，实现画面内容的平行拖动（类似编辑器中的平移视图）。
 */
void Camera::Pan(float dx, float dy)
{
    // 平移相机
    m_position += m_right * dx + m_up * dy;
    m_target += m_right * dx + m_up * dy;
}

/*
 * 以固定步长调整视野缩放（Zoom）
 *
 * amount>0 缩小 FOV（画面放大），amount<0 扩大 FOV（画面缩小）。
 * FOV 被限制在 [1, 45] 度范围内，防止过小的 FOV 导致透视畸变
 * 或过大的 FOV 导致场景形变。
 */
void Camera::Zoom(float amount)
{
    // 调整缩放
    m_zoom -= amount;
    if (m_zoom < 1.0f) m_zoom = 1.0f;
    if (m_zoom > 45.0f) m_zoom = 45.0f;
    updateCameraVectors();
}

/*
 * 绕世界原点做轨道旋转（Orbit）
 *
 * 将相机位置转换到球坐标系（yaw/pitch/radius），叠加旋转角后
 * 转回笛卡尔坐标，实现相机围绕场景中心的轨道式旋转。
 * 关键点：
 *   - pitch 限制在 ±89°，避免万向节锁（极点处方向翻转）
 *   - 每次旋转后相机始终看向原点（m_target = 0）
 *   - 相机与原点距离过近（<0.001）时直接返回，防止除零
 */
void Camera::OrbitAroundOrigin(float horizontalAngle, float verticalAngle)
{
    // 计算相机到世界原点的向量
    glm::vec3 toOrigin = -m_position;
    float radius = glm::length(toOrigin);
    
    // 如果距离原点太近，避免除零错误
    if (radius < 0.001f) {
        return;
    }
    
    // 将相机位置转换为球坐标系
    float currentYaw = atan2(m_position.z, m_position.x);
    float currentPitch = asin(m_position.y / radius);
    
    // 应用旋转角度
    float newYaw = currentYaw + glm::radians(horizontalAngle);
    float newPitch = currentPitch + glm::radians(verticalAngle);
    
    // 限制俯仰角，避免万向节锁
    const float pitchLimit = glm::radians(89.0f);
    if (newPitch > pitchLimit) newPitch = pitchLimit;
    if (newPitch < -pitchLimit) newPitch = -pitchLimit;
    
    // 将球坐标转换回笛卡尔坐标
    glm::vec3 newPosition;
    newPosition.x = radius * cos(newPitch) * cos(newYaw);
    newPosition.y = radius * sin(newPitch);
    newPosition.z = radius * cos(newPitch) * sin(newYaw);
    
    // 更新相机位置
    m_position = newPosition;
    
    // 更新相机方向（始终看向原点）
    m_front = glm::normalize(-m_position);
    m_right = glm::normalize(glm::cross(m_front, m_world_up));
    m_up = glm::normalize(glm::cross(m_right, m_front));
    
    // 更新目标点为原点（0,0,0），确保轨道旋转时始终看向世界原点
    m_target = glm::vec3(0.0f, 0.0f, 0.0f);
}