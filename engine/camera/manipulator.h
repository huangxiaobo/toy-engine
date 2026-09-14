#ifndef __MANIPULATOR_H__
#define __MANIPULATOR_H__

class Camera;

/*
 * 相机操控器抽象基类（CameraManipulator）
 *
 * 业界（OSG）的"相机-操控器"分层：Camera 只承载状态（位置/朝向/投影参数），
 * 不包含任何输入与控制逻辑；所有"如何把用户输入转换为相机姿态变化"的规则
 * 由独立的 Manipulator 实现，切换交互模式 = 更换操控器，Camera 本身不改。
 *
 * 本类定义操控器的统一契约：
 *   - SetCamera/GetCamera：操控器作用于哪个相机（OSG 绑定模式），
 *     由渲染器在初始化时绑定，此后该操控器负责更新此相机的姿态
 *   - ApplyToCamera()：把操控器内部参数（如轨道的半径/角度）落回相机，
 *     子类每次状态变更后自行调用
 */
class CameraManipulator {
public:
    virtual ~CameraManipulator() = default;

    // 绑定要操控的目标相机（渲染器初始化时调用一次）
    void SetCamera(Camera *camera) { m_camera = camera; }
    Camera *GetCamera() const { return m_camera; }

    // 将操控器内部参数写回相机（位置/朝向），子类实现
    virtual void ApplyToCamera() = 0;

protected:
    Camera *m_camera = nullptr; // 被操控的相机（仅绑定，不拥有生命周期）
};

#endif // __MANIPULATOR_H__