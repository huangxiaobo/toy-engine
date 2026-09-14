#ifndef __CAMERA_H__
#define __CAMERA_H__
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace std;

/*
 * 相机类（纯状态容器）
 *
 * 按业界（OSG）"相机-操控器"分层设计：本类只承载相机状态——
 *   位置（m_position）+ 朝向（m_orientation 四元数）——并提供
 *   视图矩阵生成（GetViewMatrix）与状态访问/修改接口。
 *
 * 所有"输入 → 姿态变化"的交互逻辑（轨道 / 平移 / 缩放等）不在此实现，
 * 而是由 CameraManipulator 子类（如 OrbitManipulator）负责；相机只提供
 * SetPosition/SetOrientation 让操控器写入最新姿态。
 *
 * 朝向用四元数表示而非 front/right/up 三向量：旋转紧凑、可任意组合、
 * 无万向节锁（与业界一致）。视图矩阵由四元数推导，保证与朝向严格一致。
 */
class Camera
{
public:
    Camera();
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 world_up);
    ~Camera();

    std::string GetName() const;

    // ---- 状态访问 ----
    glm::vec3 GetPosition() const { return m_position; }
    glm::quat GetOrientation() const { return m_orientation; }

    // ---- 状态修改（供操控器写入最新姿态） ----
    void SetPosition(const glm::vec3 &position);
    // 设置朝向四元数（相机前方 = 四元数旋转后的 -Z；由操控器计算并传入）
    void SetOrientation(const glm::quat &orientation);
    // 以「位置-目标-上方向」重建朝向（构造时 / 配置加载时使用）
    void LookAt(const glm::vec3 &target, const glm::vec3 &world_up);

    // 视图矩阵：由「位置 + 四元数朝向」推导（相机前方 -Z 经四元数旋转）
    glm::mat4 GetViewMatrix() const;

public:
    std::string m_name;
    glm::vec3 m_position;   // 相机位置

private:
    glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // 朝向（单位四元数）
};

#endif