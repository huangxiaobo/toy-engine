#include "animation.h"

#include <glm/gtc/constants.hpp>

#include "anim_target.h"

/*
 * 绑定动画到目标对象：记录接口指针，动画不拥有目标（生命周期归 Renderer）。
 */
Animation::Animation(IAnimTarget *target)
    : m_target(target) {
}

/*
 * 添加通道：同属性已存在则覆盖参数（保持单条），否则追加。
 * 无论新增还是覆盖，通道都置为活跃参与求值。
 */
void Animation::AddChannel(const AnimChannel &channel) {
    for (auto &ch : m_channels) {
        if (ch.property == channel.property) {
            ch = channel;
            ch.active = true;
            return;
        }
    }
    m_channels.push_back(channel);
    m_channels.back().active = true;
}

/*
 * 查询指定属性是否配置了活跃通道
 */
bool Animation::HasChannel(AnimProperty prop) const {
    for (const auto &ch : m_channels) {
        if (ch.property == prop && ch.active) {
            return true;
        }
    }
    return false;
}

/*
 * 读取活跃通道（返回可修改指针供面板原地编辑参数，立即生效无需写回）
 */
AnimChannel *Animation::GetChannel(AnimProperty prop) {
    for (auto &ch : m_channels) {
        if (ch.property == prop && ch.active) {
            return &ch;
        }
    }
    return nullptr;
}

/*
 * 每帧推进动画：累加时间后按曲线类型求值，写回目标对象属性
 *
 *   Sine：value = center + amplitude * sin(2π f t)
 *   Spin：value = center + speed * t
 *   Orbit：x = cx + rx·cos(2π f t)，z = cz + rz·sin(2π f t)，y 固定为 cy
 *          （amplitude 的 x/z 分量即轨道半径，90° 相位差形成圆周）
 * 目标不支持该属性（CanAnimate 为 false）时静默跳过，
 * 配置阶段即应通过能力查询过滤，此处为兜底防御。
 */
void Animation::Update(float dt) {
    if (!m_enabled || m_target == nullptr) {
        return;
    }
    m_time += dt;

    for (const auto &ch : m_channels) {
        if (!ch.active) {
            continue;
        }
        if (!m_target->CanAnimate(ch.property)) {
            continue;
        }

        glm::vec3 value(0.0f);
        if (ch.curve == AnimCurveType::Sine) {
            const float phase = glm::two_pi<float>() * ch.frequency * m_time;
            value = ch.center + ch.amplitude * glm::sin(phase);
        } else if (ch.curve == AnimCurveType::Spin) {
            value = ch.center + ch.speed * m_time;
        } else if (ch.curve == AnimCurveType::Orbit) {
            // 水平面圆周：x 走余弦、z 走正弦，相位差 π/2；y 保持轨道中心高度
            const float angle = glm::two_pi<float>() * ch.frequency * m_time;
            value.x = ch.center.x + ch.amplitude.x * glm::cos(angle);
            value.y = ch.center.y;
            value.z = ch.center.z + ch.amplitude.z * glm::sin(angle);
        } else {
            continue;
        }

        m_target->SetAnimValue(ch.property, value);
    }
}

void Animation::SetEnabled(bool enabled) {
    m_enabled = enabled;
}

bool Animation::IsEnabled() const {
    return m_enabled;
}