#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include <string>
#include <vector>
#include "anim_types.h"

class IAnimTarget;

/*
 * 通用动画（Animation）
 *
 * 动画核心：不关心目标对象具体类型（模型/灯光/相机），只持有
 * IAnimTarget 接口指针 + 一组属性通道。每帧 Update 推进时间，
 * 对每条活跃通道按曲线类型求值，经目标接口写回属性（见 anim_target.h）。
 *
 * 与旧 TransformAnimation 的关系：由"写死三个变换通道"泛化为
 * "通道列表"。通道可通过 AddChannel 任意添加/替换，新增属性类型
 * 只需新枚举值 + 目标类一行 case，动画核心零改动。
 */
class Animation {
public:
    explicit Animation(IAnimTarget *target);

    // 添加通道：同属性已存在则覆盖参数，否则追加（active 置真）
    void AddChannel(const AnimChannel &channel);
    // 是否配置了指定属性的活跃通道（面板/类型摘要查询）
    bool HasChannel(AnimProperty prop) const;
    // 读取通道（面板原地编辑参数用），无此属性返回 nullptr
    AnimChannel *GetChannel(AnimProperty prop);

    // 每帧推进动画（dt 单位：秒）
    void Update(float dt);

    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    void SetName(const std::string &name) { m_name = name; }
    const std::string &GetName() const { return m_name; }
    // 绑定的目标对象（面板按目标筛选动画用）
    IAnimTarget *GetTarget() const { return m_target; }

private:
    IAnimTarget *m_target;                       // 绑定的目标（弱引用，不拥有）
    std::string m_name;                          // 动画名称（world.yaml animations[].name）
    float m_time = 0.0f;                         // 累计时间（秒）
    bool m_enabled = true;                       // 总开关
    std::vector<AnimChannel> m_channels;         // 属性通道列表
};

#endif // __ANIMATION_H__