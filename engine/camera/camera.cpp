#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
}

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 world_up)
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
    return glm::lookAt(m_position, m_position +  m_front * 10.0f, m_world_up);
}
Camera::~Camera()
{
}
