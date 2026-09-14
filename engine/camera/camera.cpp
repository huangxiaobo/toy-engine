#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

/*
 * Camera 实现（状态容器版本）
 *
 * 旧的 Camera 同时承载了轨道/平移/缩放等输入逻辑（OrbitAroundOrigin/Pan/Zoom）
 * 与自由第一人称逻辑（ProcessKeyboard/ProcessMouseMovement），两类逻辑互相
 * 覆盖同一组 front/right/up 状态，且后者从未被调用。重构后相机只剩状态与
 * 视图矩阵生成，交互逻辑移入 OrbitManipulator（见 manipulator.h）。
 */

// 默认构造：位置与朝向留默认值（位置原点、朝向单位四元数 = 看向 -Z）
Camera::Camera()
{
}

/*
 * 参数化构造：以「位置-目标-世界上方向」初始化相机
 *
 * 与旧版语义一致，用于从 CameraConfig 加载摄像机：
 *   - position 相机位置
 *   - target   初始注视目标（用于推导朝向）
 *   - world_up 世界上方向
 * 内部把"朝向 = 目标 - 位置"换算为四元数存储（见 LookAt）。
 */
Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 world_up)
    : m_position(position)
{
    LookAt(target, world_up);
}

Camera::~Camera()
{
}

std::string Camera::GetName() const {
    return m_name;
}

// 移动相机位置（纯平移，不改变朝向；操控器在需要时自行调整朝向）
void Camera::SetPosition(const glm::vec3 &position) {
    m_position = position;
}

// 由操控器写入朝向四元数（约定：相机前方 = 四元数旋转后的 -Z）
void Camera::SetOrientation(const glm::quat &orientation) {
    m_orientation = orientation;
}

/*
 * 以「目标点 + 世界上方向」重建朝向
 *
 * 依据 position 与 target 推导视线方向 dir，再用 dir 与 world_up 构造
 * 四元数：glm::quatLookAt(dir, up) 生成使相机前方（-Z）正对 dir 的旋转。
 * 视线与世界上方向几乎平行时（往正上/正下看）改为使用 Z 轴作为上方向，
 * 避免叉积退化为零向量导致 NaN（与旧 OrbitAroundOrigin 的 ±89° 限制同源）。
 */
void Camera::LookAt(const glm::vec3 &target, const glm::vec3 &world_up) {
    const glm::vec3 dir = glm::normalize(target - m_position);

    // 视线与 up 平行时退化：改用 Z 轴充当上方向
    const glm::vec3 safeUp = (std::fabs(glm::dot(dir, world_up)) > 0.999f)
                                 ? glm::vec3(0.0f, 0.0f, 1.0f)
                                 : world_up;
    m_orientation = glm::quatLookAt(dir, safeUp);
}

/*
 * 视图矩阵
 *
 * 由「位置 + 四元数朝向」构造 lookAt：
 *   - 相机前方 = 四元数旋转后的 -Z（OpenGL 相机默认看向 -Z）
 *   - 上方向   = 四元数旋转后的 +Y
 * 分别作为 lookAt 的注视点方向与上向量，保证朝向与矩阵严格一致。
 */
glm::mat4 Camera::GetViewMatrix() const {
    // 相机本地 -Z 轴经四元数旋转后即世界空间下的观察方向
    const glm::vec3 front = m_orientation * glm::vec3(0.0f, 0.0f, -1.0f);
    // 相机本地 +Y 轴经四元数旋转后即世界空间下的上方向
    const glm::vec3 up = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::lookAt(m_position, m_position + front, up);
}