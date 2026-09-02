#include "axis.h"

// ImGui 绘制：需要 DrawList 与字体
#include <imgui.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>

/*
 * 屏幕空间坐标轴 gizmo 实现
 *
 * 数学原理（与 erhe 的 ImViewGuizmo::draw_rotate 一致）：
 *   相机 world 矩阵 = translate(eyePos) * rotate(cameraRot)，
 *   视图矩阵 = inverse(world)。gizmo 只需要方向信息，因此取视图矩阵的
 *   旋转子矩阵（3x3）作为 gizmo 视图矩阵，配合正交投影
 *   ortho(-1,1,-1,1,-100,100) 将 ±X/±Y/±Z 单位方向投到 NDC，
 *   再映射到以 center 为中心、gizmoDiameter 为直径的屏幕区域。
 *   这样坐标系始终朝向观察者，且只随相机旋转改变朝向。
 */

namespace {
    // gizmo 基准直径（逻辑像素，实际直径 = 基准直径 * scale）
    constexpr float baseSize = 256.0f;
    // 轴长度（单位方向向量的缩放系数，控制把手离中心距离）
    constexpr float lineLength = 0.5f;
    // 轴线的像素宽度
    constexpr float lineWidth = 4.0f;
    // 轴端圆点把手的半径（逻辑像素）
    constexpr float circleRadius = 15.0f;
    // 深度淡化系数：轴朝向越偏离视线方向，透明度越低
    constexpr float fadeFactor = 0.25f;
    // 三轴颜色（X 红 / Y 绿 / Z 蓝），与 erhe ImViewGuizmo 一致
    constexpr std::array<ImU32, 3> axisColors = {
        IM_COL32(230,  51,  51, 255),  // X
        IM_COL32( 51, 230,  51, 255),  // Y
        IM_COL32( 51, 128, 255, 255)   // Z
    };
    // 轴标签与配色
    constexpr std::array<const char*, 3> axisLabels = {"X", "Y", "Z"};
    const ImU32 labelColorPos = IM_COL32(255, 255, 255, 255); // 正半轴白字
    const ImU32 labelColorNeg = IM_COL32(  0,   0,   0, 255); // 负半轴黑字
    constexpr float labelSize = 1.0f;

    // 世界坐标轴单位方向
    constexpr std::array<glm::vec3, 3> axisVectors = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    };

    // gizmo 单轴描述：id 0-5 (+X,-X,+Y,-Y,+Z,-Z)，深度为视图空间 z
    struct GizmoAxis {
        int       id;        // 0-5
        int       axisIndex; // 0=X, 1=Y, 2=Z
        float     depth;     // 视图空间深度
        glm::vec3 direction; // 世界空间单位方向
    };
} // namespace

Axis::Axis() = default;
Axis::~Axis() = default;

void Axis::SetScale(float scale)
{
    m_scale = scale;
}

void Axis::Draw(const glm::mat4& viewMatrix, const ImVec2& center)
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const float gizmoDiameter = baseSize * m_scale;

    // 只取视图矩阵的旋转部分作为 gizmo 的视图矩阵（去掉平移，方向才正确）
    const glm::mat4 gizmoViewMatrix = glm::mat4(glm::mat3(viewMatrix));
    const glm::mat4 gizmoProjectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -100.0f, 100.0f);
    const glm::mat4 gizmoMvp = gizmoProjectionMatrix * gizmoViewMatrix;

    // 构建六轴（±X/±Y/±Z），记录各自在视图空间的深度
    std::array<GizmoAxis, 6> axes;
    for (int i = 0; i < 3; ++i) {
        const glm::vec3 dir = axisVectors[i];
        axes[i * 2 + 0] = { i * 2 + 0, i, (gizmoViewMatrix * glm::vec4{ dir, 0.0f }).z,  dir};
        axes[i * 2 + 1] = { i * 2 + 1, i, (gizmoViewMatrix * glm::vec4{-dir, 0.0f }).z, -dir};
    }

    // 按深度升序排序（先画近处，后画远处，避免远处轴盖住近处轴）
    std::sort(axes.begin(), axes.end(),
        [](const GizmoAxis& a, const GizmoAxis& b) { return a.depth < b.depth; });

    // 世界坐标 -> gizmo 屏幕坐标（NDC 映射到以 center 为中心、gizmoDiameter 为直径的正方形区域）
    auto worldToScreen = [&](const glm::vec3& worldPos) -> ImVec2 {
        const glm::vec4 clipPos = gizmoMvp * glm::vec4{ worldPos, 1.0f };
        if (clipPos.w == 0.0f) {
            return { -FLT_MAX, -FLT_MAX };
        }
        const glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        return {
            center.x + ndc.x * (gizmoDiameter / 2.0f),
            center.y - ndc.y * (gizmoDiameter / 2.0f)
        };
    };

    const glm::vec3 origin{ 0.0f, 0.0f, 0.0f };

    // 绘制六轴：中心 -> 圆点把手（线在圆边缘截止）
    for (const auto& axis : axes) {
        // 按深度淡化颜色：深度 +1（正对相机）全亮，深度 0 附近透明度降到 25%
        float factor = glm::mix(fadeFactor, 1.0f, (axis.depth + 1.0f) * 0.5f);
        ImVec4 baseColor = ImGui::ColorConvertU32ToFloat4(axisColors[axis.axisIndex]);
        ImVec4 fadedColor{ baseColor.x, baseColor.y, baseColor.z, baseColor.w * factor };
        ImU32 finalColor = ImGui::ColorConvertFloat4ToU32(fadedColor);

        const ImVec2 originPos = worldToScreen(origin);
        const ImVec2 handlePos = worldToScreen(axis.direction * lineLength);

        // 线从中心画到把手，但在圆点把手边缘处截止（避免线段穿入圆内）
        ImVec2 lineDir{ handlePos.x - originPos.x, handlePos.y - originPos.y };
        float lineLengthPx = std::sqrt(lineDir.x * lineDir.x + lineDir.y * lineDir.y) + 1e-6f;
        lineDir.x /= lineLengthPx;
        lineDir.y /= lineLengthPx;
        const float scaledCircleRadius = circleRadius * m_scale;
        const ImVec2 lineEndPos{
            handlePos.x - lineDir.x * scaledCircleRadius,
            handlePos.y - lineDir.y * scaledCircleRadius
        };

        drawList->AddLine(originPos, lineEndPos, finalColor, lineWidth * m_scale);
        drawList->AddCircleFilled(handlePos, scaledCircleRadius, finalColor);
    }

    // 绘制 X/Y/Z 标签（仅画朝向相机一侧的轴，背向相机的轴被跳过）
    ImFont* font = ImGui::GetFont();
    const float scaledFontSize = ImGui::GetFontSize() * m_scale * labelSize;
    for (const auto& axis : axes) {
        if (axis.depth < -0.1f) {
            continue;
        }
        const ImVec2 textPos = worldToScreen(axis.direction * lineLength);
        const char* label = axisLabels[axis.axisIndex];
        const bool isPositive = (axis.id & 1) == 0; // 偶数 id 为正半轴
        const ImVec2 textSize = font->CalcTextSizeA(scaledFontSize, FLT_MAX, 0.0f, label);
        drawList->AddText(
            font,
            scaledFontSize,
            { textPos.x - textSize.x * 0.5f, textPos.y - textSize.y * 0.5f },
            isPositive ? labelColorPos : labelColorNeg,
            label
        );
    }
}