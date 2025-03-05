#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <glm/glm.hpp>

enum CameraMoveType
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
public:

    Camera();
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 world_up);
    ~Camera();

    void Roll(float angle);
    void Pitch(float angle);
    void Yaw(float angle);
    void Slide(float du, float dv, float dn);

    glm::mat4 GetViewMatrix();

    glm::vec3 GetEyePosition() { return m_position; }

public:
    glm::vec3 m_position; // 摄像机位置
    glm::vec3 m_target;

    glm::vec3 m_front;    // 摄像机前方
    glm::vec3 m_right;    // 摄像机右边
    glm::vec3 m_up;       // 摄像机上方
    glm::vec3 m_world_up; // 世界空间的上方
};

#endif