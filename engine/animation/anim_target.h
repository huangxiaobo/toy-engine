#ifndef __ANIM_TARGET_H__
#define __ANIM_TARGET_H__

#include <string>
#include <glm/glm.hpp>
#include "anim_types.h"

/*
 * 可动画对象接口（IAnimTarget）
 *
 * 动画系统的"绑定层"抽象：Animation 每帧对每条通道求值后调用
 * SetAnimValue 写回目标对象。模型/灯光/相机都实现本接口接入动画，
 * 动画核心无需知道目标具体类型（业界"绑定+属性"架构的最小实现）。
 *
 * 实现约定：
 *   - CanAnimate 返回此属性是否支持（灯光不支持缩放、方向光不支持位置）
 *   - SetAnimValue 内用 switch 分发到对象自己的属性 setter
 *   - 对象生命周期归 Renderer，接口只持有裸指针（弱引用）
 */
class IAnimTarget {
public:
    virtual ~IAnimTarget() = default;

    // 目标名称（日志与面板显示）
    virtual std::string GetName() const = 0;

    // 是否支持动画驱动指定属性（不支持的属性动画静默跳过）
    virtual bool CanAnimate(AnimProperty prop) const = 0;

    // 写回属性动画值（值语义统一 vec3，标量属性取 x）
    virtual void SetAnimValue(AnimProperty prop, const glm::vec3 &value) = 0;
};

#endif // __ANIM_TARGET_H__