#ifndef __ANIM_TYPES_H__
#define __ANIM_TYPES_H__

#include <glm/glm.hpp>

/*
 * 动画属性枚举（AnimProperty）
 *
 * 动画核心（Animation）不认识具体的对象类型（模型/灯光/相机），
 * 只认识"属性"：每一条通道绑定一个枚举属性，求值后经 IAnimTarget
 * 写回目标对象。新增可动画属性 = 加一个枚举值 + 目标类一行 case，
 * 动画核心和数据层零改动（业界"属性绑定"的最小实现）。
 *
 * 值语义统一为 vec3（强度只取 x 分量），曲线求值无需区分类型。
 */
enum class AnimProperty {
    Position,        // 位置（模型/点光/聚光支持；方向光无位置）
    Scale,           // 缩放（仅模型）
    Rotation,        // 旋转：模型=欧拉角（度）；方向光/聚光=转到方向向量的欧拉角
    LightColor,      // 灯光颜色（R,G,B）
    LightIntensity,  // 灯光强度（取 vec3.x）
};

/*
 * 曲线类型（AnimCurveType）
 *
 * 程序化无限动画，无需关键帧资产：
 *   - Sine：正弦振荡  value = center + amplitude * sin(2π f t)
 *   - Spin：匀速变化  value = center + speed * t（旋转器绕轴转/灯光扫动）
 *   - Orbit：绕 Y 轴圆周轨道  x = cx + rx·cos(2π f t)，z = cz + rz·sin(2π f t)，y 固定
 *            （点光源无朝向，位置绕 Y 轴画圈是其"旋转"语义）
 * 未来需关键帧回放时再追加 Keyframe 类型。
 */
enum class AnimCurveType {
    Sine,   // 正弦振荡
    Spin,   // 匀速推进
    Orbit,  // 绕 Y 轴圆周轨道
    None,   // 未配置（通道占位）
};

/*
 * 单条动画通道（AnimChannel）
 *
 * 一个属性 + 一组曲线参数。参数按 值语义统一为 vec3：
 * 强度等标量属性只使用 x 分量，其余分量保持 0。
 */
struct AnimChannel {
    AnimProperty property = AnimProperty::Position;   // 绑定属性
    AnimCurveType curve = AnimCurveType::Sine;        // 曲线类型

    glm::vec3 center{};      // 振荡中心 / 匀速起点
    glm::vec3 amplitude{};   // 单侧振幅（Sine）
    float frequency = 1.0f;  // 频率 Hz（Sine）
    glm::vec3 speed{};       // 角速度（度/秒）或推进速度（Spin）

    bool active = false;     // 通道是否参与求值
};

#endif // __ANIM_TYPES_H__