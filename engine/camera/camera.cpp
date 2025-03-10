#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
}

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

glm::mat4 Camera::GetViewMatrix()
{
    // glm::LookAt函数需要一个相机世界空间位置、一个目标位置、世界空间中的上向量,创建作为观察矩阵
    return glm::lookAt(m_position, m_position + m_front * 10.0f, m_world_up);
}
void Camera::ProcessKeyboard(CameraMoveType direction, float deltaTime)
{
    float velocity = m_move_speed * deltaTime;
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
void Camera::updateCameraVectors()
{
    // 更新相机的前向量和右向量
    m_front = glm::normalize(m_target - m_position);
    m_right = glm::normalize(glm::cross(m_front, m_world_up));
}
Camera::~Camera()
{
}

// ... 已有代码 ...

void Camera::Roll(float angle)
{
    // 创建一个绕相机前向向量旋转的旋转矩阵
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), m_front);

    // 使用旋转矩阵更新相机的上向量和右向量
    m_up = glm::vec3(rotation * glm::vec4(m_up, 0.0f));
    m_right = glm::vec3(rotation * glm::vec4(m_right, 0.0f));
}