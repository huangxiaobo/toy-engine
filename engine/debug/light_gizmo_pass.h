#pragma once

#include <glm/glm.hpp>

#include <vector>

class DebugDraw;
class Light;

// 灯光 gizmo 调试可视化：遍历灯光列表收集朝向/位置线段并一次性提交，
// 开关关闭时整段跳过，不收集也不提交
class LightGizmoPass {
public:
    void Render(DebugDraw &debug_draw,
                const std::vector<Light *> &lights,
                const glm::mat4 &projection,
                const glm::mat4 &view);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetRangeEnabled(bool enabled) { m_range_enabled = enabled; }
    bool IsRangeEnabled() const { return m_range_enabled; }

private:
    void collectGizmos(DebugDraw &debug_draw, const std::vector<Light *> &lights);
    void collectRangeGizmos(DebugDraw &debug_draw, const std::vector<Light *> &lights);

    bool m_enabled = false;
    bool m_range_enabled = false;
};
