#include "light_gizmo_pass.h"

#include "debug_draw.h"
#include "../light/light.h"

void LightGizmoPass::Render(DebugDraw &debug_draw,
                            const std::vector<Light *> &lights,
                            const glm::mat4 &projection,
                            const glm::mat4 &view) {
    if (!m_enabled) {
        return;
    }

    collectGizmos(debug_draw, lights);
    if (m_range_enabled) {
        collectRangeGizmos(debug_draw, lights);
    }
    // 批量提交：上传本帧全部 gizmo 线段，一次 glDrawArrays(GL_LINES) 绘制（内部自动 Clear）
    debug_draw.Render(projection, view);
}

void LightGizmoPass::collectGizmos(DebugDraw &debug_draw, const std::vector<Light *> &lights) {
    for (auto light: lights) {
        if (light == nullptr) {
            continue;
        }
        switch (light->GetLightType()) {
            case LightTypeSpot:
                debug_draw.DrawSpotLight(static_cast<SpotLight *>(light));
                break;
            case LightTypeDirection:
                debug_draw.DrawDirectionLight(static_cast<DirectionLight *>(light));
                break;
            case LightTypePoint:
                debug_draw.DrawPointLight(static_cast<PointLight *>(light));
                break;
            default:
                break;
        }
    }
}

void LightGizmoPass::collectRangeGizmos(DebugDraw &debug_draw, const std::vector<Light *> &lights) {
    for (auto light: lights) {
        if (light == nullptr) {
            continue;
        }
        switch (light->GetLightType()) {
            case LightTypePoint: {
                auto *pointLight = static_cast<PointLight *>(light);
                const float radius = DebugDraw::ComputePointLightRadius(pointLight);
                debug_draw.DrawSphereWireframe(pointLight->m_position, radius,
                                               glm::vec3(0.3f, 1.0f, 0.6f), 24);
                break;
            }
            case LightTypeSpot: {
                auto *spotLight = static_cast<SpotLight *>(light);
                // 用外锥角（OuterCutoff）绘制完整半影锥体范围
                debug_draw.DrawConeWireframe(spotLight->m_position, spotLight->m_direction,
                                             spotLight->m_outer_cutoff, 30.0f,
                                             glm::vec3(1.0f, 0.75f, 0.2f), 24);
                break;
            }
            default:
                break;
        }
    }
}
