#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <glm/glm.hpp>

enum CameraMoveType
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
};

class Camera
{
public:

    Camera();
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 world_up);
    ~Camera();

    // 滚转角(Roll)：沿z轴旋转的角（对于摄像机而言，一般不关心这个)
    void Roll(float angle);
    // 俯仰角(Pitch)：沿x轴旋转的角，从上往下看的角
    void Pitch(float angle);
    // 偏航角(Yaw)：沿y轴旋转的角，从左往右看的角
    void Yaw(float angle);
    void Slide(float du, float dv, float dn);

    glm::mat4 GetViewMatrix();

    glm::vec3 GetEyePosition() { return m_position; }

    void ProcessKeyboard   (
        CameraMoveType direction,
        float deltaTime
    );
    void ProcessMouseMovement
    (
        float xoffset,
        float yoffset,
        bool constrainPitch = true
    );
    void ProcessMouseScroll(float yoffse);
private:
    void updateCameraVectors();

public:
    glm::vec3 m_position; // 摄像机位置
    glm::vec3 m_target;

    glm::vec3 m_front;    // 摄像机前方
    glm::vec3 m_right;    // 摄像机右边
    glm::vec3 m_up;       // 摄像机上方
    glm::vec3 m_world_up; // 世界空间的上方

    float m_move_speed; // 摄像机移动速度
    float m_mouse_sensitivity; // 鼠标灵敏度
    float m_zoom; // 摄像机缩放
};

#endif