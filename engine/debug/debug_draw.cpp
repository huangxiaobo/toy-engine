#include "debug_draw.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstddef>

#include "../technique/technique.h"
#include "../light/light.h"

/*
 * 构造函数：所有成员在头文件内已初始化（m_effect=nullptr, m_vao=0 等）
 */
DebugDraw::DebugDraw() = default;

/*
 * 析构函数：释放自管的 OpenGL 资源
 * 注意：m_effect（着色器）不归本类所有，由 Renderer 统一释放，这里不处理。
 */
DebugDraw::~DebugDraw() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
}

/*
 * 初始化：绑定调用方共享的调试着色器，并配置 VAO 顶点布局
 *
 * 顶点格式 DebugVertex = { position(vec3), color(vec3) }：
 *   - location 0 = position，偏移 0
 *   - location 1 = color，偏移 offsetof(color)
 * 两个属性都是每顶点连续布局（stride = sizeof(DebugVertex)）。
 */
void DebugDraw::Init(Technique *effect) {
    m_effect = effect;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // position 属性（location 0）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
                          reinterpret_cast<void *>(offsetof(DebugVertex, position)));
    glEnableVertexAttribArray(0);

    // color 属性（location 1）
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
                          reinterpret_cast<void *>(offsetof(DebugVertex, color)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
 * 追加一条线段（两个顶点）到本帧收集列表
 */
void DebugDraw::DrawLine(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color) {
    m_vertices.push_back({from, color});
    m_vertices.push_back({to, color});
}

/*
 * 绘制线框球体：X/Y/Z 三个正交平面上的圆环
 *
 * 每个圆环由 segments 条短线段闭合组成，三个平面正交叠加即可勾勒出球体轮廓，
 * 无需法线/纹理，纯调试语义。
 */
void DebugDraw::DrawSphereWireframe(const glm::vec3 &center, float radius,
                                    const glm::vec3 &color, int segments) {
    // 在由 n1/n2 张成的平面上绘制一个半径为 radius 的闭合圆环
    auto drawRing = [&](const glm::vec3 &n1, const glm::vec3 &n2) {
        for (int i = 0; i < segments; ++i) {
            const float a = 2.0f * static_cast<float>(M_PI) * i / segments;
            const float b = 2.0f * static_cast<float>(M_PI) * (i + 1) / segments;
            const glm::vec3 p1 = center + n1 * (radius * std::cos(a)) + n2 * (radius * std::sin(a));
            const glm::vec3 p2 = center + n1 * (radius * std::cos(b)) + n2 * (radius * std::sin(b));
            DrawLine(p1, p2, color);
        }
    };

    drawRing(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // XY 平面
    drawRing(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // XZ 平面
    drawRing(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // YZ 平面
}

/*
 * 绘制线框圆锥：锥顶 + 母线 + 底面圆环
 *
 * 锥轴沿 direction 方向延伸 length，半角 angleDeg 决定底面圆半径
 * （radius = length * tan(angleDeg)）。用于调试锥体范围的通用原语。
 */
void DebugDraw::DrawConeWireframe(const glm::vec3 &apex, const glm::vec3 &direction,
                                  float angleDeg, float length,
                                  const glm::vec3 &color, int segments) {
    // 构造世界空间正交基：zAxis = 光照方向
    glm::vec3 xAxis, yAxis, zAxis;
    BuildBasis(direction, xAxis, yAxis, zAxis);

    const float radius = length * std::tan(glm::radians(angleDeg));
    const glm::vec3 baseCenter = apex + zAxis * length;

    // 生成底面圆环点（世界空间）
    std::vector<glm::vec3> ring;
    ring.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        const float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        ring.push_back(baseCenter
                       + xAxis * (radius * std::cos(angle))
                       + yAxis * (radius * std::sin(angle)));
    }

    // 母线：锥顶 -> 底面圆环点（闭合）
    for (int i = 0; i < segments; ++i) {
        DrawLine(apex, ring[i], color);
    }
    // 底面圆环：相邻点成对（含尾 -> 首闭合）
    for (int i = 0; i < segments; ++i) {
        DrawLine(ring[i], ring[(i + 1) % segments], color);
    }
}

/*
 * 绘制带箭头头的线段：主轴线 + 顶端两条斜线箭头
 *
 * 箭头头部长约为主轴线长度的 15%，张开角约 25°。
 * 用于调试"方向"类信息的通用原语（如方向光指示线）。
 */
void DebugDraw::DrawArrow(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color) {
    DrawLine(from, to, color);

    const glm::vec3 span = to - from;
    const float totalLen = glm::length(span);
    if (totalLen < 1e-6f) {
        return;
    }
    const glm::vec3 dir = span / totalLen;

    // 箭头头：在主轴末端向两侧张开
    const float headLen = 0.15f * totalLen;
    // 选取与主轴不平行的辅助向量构造箭头两翼（近似但足够调试用）
    glm::vec3 side = std::fabs(dir.y) > 0.999f ? glm::vec3(1.0f, 0.0f, 0.0f)
                                               : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 wing = glm::normalize(glm::cross(dir, side));

    const glm::vec3 tip = to;
    const glm::vec3 base = to - dir * headLen;
    // 两翼各旋转 ±25°，画两条斜线
    const float spread = 0.4663f; // tan(25°)，控制箭头张开角度
    const glm::vec3 w1 = base + wing * (headLen * spread);
    const glm::vec3 w2 = base - wing * (headLen * spread);
    DrawLine(tip, w1, color);
    DrawLine(tip, w2, color);
}

/*
 * 绘制轴对齐线框盒：min/max 为世界空间对角点，12 条边勾勒
 *
 * 具体边集合：底面 4 条 + 顶面 4 条 + 连接上下底的竖棱 4 条。
 * 用于拾取高亮、包围盒可视化等需要"框住一个区域"的调试场景。
 */
void DebugDraw::DrawBoxWireframe(const glm::vec3 &min, const glm::vec3 &max, const glm::vec3 &color) {
    // 八个角点（按底/顶两类枚举，便于成对连线）
    const glm::vec3 corners[8] = {
        glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z),  // 底面
        glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z),
        glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z),  // 顶面
    };

    // 底面闭合环（0-1-2-3-0）
    for (int i = 0; i < 4; ++i) {
        DrawLine(corners[i], corners[(i + 1) % 4], color);
    }
    // 顶面闭合环（4-5-6-7-4）
    for (int i = 4; i < 8; ++i) {
        DrawLine(corners[i], corners[(i == 7) ? 4 : (i + 1)], color);
    }
    // 四根竖棱（底面点 <-> 对应顶面点）
    for (int i = 0; i < 4; ++i) {
        DrawLine(corners[i], corners[i + 4], color);
    }
}

/*
 * 绘制点光源 gizmo（放射线球面）
 *
 * 从光源位置沿斐波那契球面均匀分布的 128 个方向放射线段到影响边界，
 * 勾勒出光照衰减范围轮廓（对应业界点光源 gizmo 惯例）。
 * 半径每帧由衰减系数反推，衰减参数在编辑器中变化后自动跟随。
 */
void DebugDraw::DrawPointLight(const PointLight *light) {
    if (light == nullptr) {
        return;
    }
    const glm::vec3 &pos = light->Position;
    const float radius = ComputePointLightRadius(light);

    // 点光源 gizmo 专属标识色：绿色。
    // 三种光型分色约定（与世界坐标系中 Unreal/Unity 等编辑器的惯例一致）：
    //   点光源 -> 绿色、聚光灯 -> 琥珀黄、方向光 -> 蓝色，便于在场景中一眼区分光型。
    const glm::vec3 pointColor(0.2f, 0.9f, 0.4f);
    const int rayCount = 128;

    // 斐波那契球面分布：y 在 [-1,1] 均匀分布（+0.5 使点避开两极），
    // 经度按黄金角累积旋转，避免相邻层点在同一条经线上聚簇
    const float goldenAngle = static_cast<float>(M_PI) * (3.0f - std::sqrt(5.0f));
    for (int i = 0; i < rayCount; ++i) {
        const float y = 1.0f - 2.0f * (i + 0.5f) / rayCount;
        const float r = std::sqrt(1.0f - y * y);
        const float theta = goldenAngle * i;
        const glm::vec3 dir(std::cos(theta) * r, y, std::sin(theta) * r);
        DrawLine(pos, pos + dir * radius, pointColor);
    }
}

/*
 * 绘制聚光灯 gizmo（双层双锥线框）
 *
 * 锥顶位于光源位置，锥轴沿光照方向延伸 length（与原实现一致取 30 单位）。
 *   - 内锥（Cutoff 半角）：实线，表示全亮范围，纯琥珀黄
 *   - 外锥（OuterCutoff 半角）：虚线（底面圆环隔段绘制），偏暗琥珀黄 = 半影过渡范围
 *   - 中轴线：锥顶 -> 内锥底面中心，纯琥珀黄
 * 顶点直接在 CPU 端用 BuildBasis 构造的世界空间正交基生成，无需 model 矩阵。
 */
void DebugDraw::DrawSpotLight(const SpotLight *light) {
    if (light == nullptr) {
        return;
    }
    const float length = 30.0f; // 锥长（与原 CreateSpotLightModelV3(30.0f) 保持一致）
    const int segments = 12;
    // 聚光灯 gizmo 专属标识色：琥珀黄（分色约定见 DrawPointLight）
    const glm::vec3 spotColor(1.0f, 0.75f, 0.2f);
    const glm::vec3 dimColor = spotColor * 0.45f; // 外锥暗调，与内锥形成明暗层级

    // 构造世界空间正交基：zAxis = 光照方向（本地 +Z 对齐 direction 的旋转语义）
    glm::vec3 xAxis, yAxis, zAxis;
    BuildBasis(light->Direction, xAxis, yAxis, zAxis);

    const glm::vec3 &apex = light->Position;
    // 由半角弧度计算底面圆半径（锥长 * 正切）
    const float innerRadius = length * std::tan(glm::radians(light->Cutoff));
    const float outerRadius = length * std::tan(glm::radians(light->OuterCutoff));

    // 生成 z=length 平面上分布 segments 个点的底面圆环（世界空间，逆时针）
    auto buildRing = [&](float radius) {
        std::vector<glm::vec3> points;
        points.reserve(segments);
        const glm::vec3 baseCenter = apex + zAxis * length;
        for (int i = 0; i < segments; ++i) {
            const float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
            points.push_back(baseCenter
                             + xAxis * (radius * std::cos(angle))
                             + yAxis * (radius * std::sin(angle)));
        }
        return points;
    };

    // 内锥（实线：母线 + 底面圆环全部闭合）
    {
        auto ring = buildRing(innerRadius);
        for (int i = 0; i < segments; ++i) {
            DrawLine(apex, ring[i], spotColor);                       // 母线
            DrawLine(ring[i], ring[(i + 1) % segments], spotColor);   // 底面圆环
        }
    }

    // 外锥（虚线：母线全部绘制 + 底面圆环只画偶数段，奇数段留空形成间断视觉）
    {
        auto ring = buildRing(outerRadius);
        for (int i = 0; i < segments; ++i) {
            DrawLine(apex, ring[i], dimColor); // 母线
        }
        for (int i = 0; i < segments; i += 2) {
            DrawLine(ring[i], ring[(i + 1) % segments], dimColor); // 只画偶数段
        }
    }

    // 中轴线：锥顶 -> 内锥底面中心
    DrawLine(apex, apex + zAxis * length, spotColor);
}

/*
 * 绘制方向光 gizmo（十字标记 + 方向线）
 *
 * 方向光没有位置概念，画在场景原点作为示意参照：
 *   - 十字标记：X/Y/Z 三轴各 ±0.5 单位，表示"光源示意位置"
 *   - 方向线：从原点沿光照方向延伸 length（与原实现 30 单位一致）
 */
void DebugDraw::DrawDirectionLight(const DirectionLight *light) {
    if (light == nullptr) {
        return;
    }
    const float length = 30.0f; // 指示线长度（与原 CreateDirectionLightModelV1(30.0f) 一致）
    // 方向光 gizmo 专属标识色：蓝色（分色约定见 DrawPointLight）
    const glm::vec3 dirColor(0.35f, 0.6f, 1.0f);

    // 十字标记：固定在场景原点（方向光无位置概念，十字作为编辑参照物不随方向旋转）
    DrawLine(glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(0.5f, 0.0f, 0.0f), dirColor);
    DrawLine(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f, 0.5f, 0.0f), dirColor);
    DrawLine(glm::vec3(0.0f, 0.0f, -0.5f), glm::vec3(0.0f, 0.0f, 0.5f), dirColor);

    // 方向线：原点沿光照方向延伸（零向量兜底不绘制，避免退化线段）
    const glm::vec3 dir = glm::normalize(light->Direction);
    if (glm::length(dir) > 1e-6f) {
        DrawLine(glm::vec3(0.0f), dir * length, dirColor);
    }
}

/*
 * 绘制单个顶点的法线线段
 *
 * 颜色按法线方向编码：将归一化法线 XYZ 各分量从 [-1,1] 映射到 [0.5,1]，
 * 使线段保持亮色（避免半黑不可见），且三个分量能直观区分法线朝向
 * （如 +X 偏红、+Y 偏绿、+Z 偏蓝）。
 */
void DebugDraw::DrawNormal(const glm::vec3 &vertexPos, const glm::vec3 &normal, float length) {
    glm::vec3 n = normal;
    const float len = glm::length(n);
    if (len < 1e-6f) {
        return; // 零法线不绘制，避免退化线段
    }
    n /= len;
    // 法线方向 -> 亮色编码：[-1,1] -> [0.5,1]，保证三通道都在可见亮度以上
    const glm::vec3 color = n * 0.25f + 0.75f;
    DrawLine(vertexPos, vertexPos + n * length, color);
}

/*
 * 每帧统一提交渲染
 *
 * 1. CPU 顶点列表非空才执行（无 gizmo 时零开销）；
 * 2. 启用调试着色器（纯顶点色，不参与光照），上传 projection/view；
 * 3. 全量重传顶点到动态 VBO（GL_DYNAMIC_DRAW）；
 * 4. glDrawArrays(GL_LINES) 一次批量绘制全部线段；
 * 5. 绘制结束自动 Clear() 释放本帧列表。
 *
 * 注意：不修改全局 OpenGL 状态（深度测试/混合等），调用方负责在
 * 场景 Pass 内调用（深度测试开启），遵循 AGENTS.md 教训2"状态不泄漏"。
 */
void DebugDraw::Render(const glm::mat4 &projection, const glm::mat4 &view) {
    if (m_vertices.empty() || m_effect == nullptr) {
        return;
    }

    m_effect->Enable();
    m_effect->SetUniform("projection", projection);
    m_effect->SetUniform("view", view);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_vertices.size() * sizeof(DebugVertex)),
                 m_vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));

    // 解绑：避免后续其它管线残留 VAO/VBO 绑定
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    Clear();
}

/*
 * 清空本帧收集的顶点列表
 */
void DebugDraw::Clear() {
    m_vertices.clear();
}

/*
 * 由衰减系数反推点光源影响半径
 *
 * 衰减公式 att = 1 / (Kc + Kl*d + Ke*d²)。求衰减到阈值 threshold 处的距离 d，
 * 作为点光源 gizmo 放射线的影响半径。
 * 阈值取 0.6（光强衰减到 60%）：影响边界收窄到光源附近，
 * 避免放射线过长遮挡场景主体（与聚光灯 30 单位锥长量级相当）。
 *
 * 求解二次方程 Ke*d² + Kl*d + (Kc - 1/threshold) = 0：
 *   - Ke > 0：取正根
 *   - Ke = 0 且 Kl > 0：线性方程 d = (1/threshold - Kc) / Kl
 *   - 无衰减（线性和二次都为零）：影响无限远，回退到场景默认尺度
 */
float DebugDraw::ComputePointLightRadius(const PointLight *light, float threshold) {
    const float Kc = light->Attenuation.Constant;
    const float Kl = light->Attenuation.Linear;
    const float Ke = light->Attenuation.Exp;
    const float target = 1.0f / threshold; // 衰减到阈值时分母的值

    if (std::fabs(Ke) > 1e-6f) {
        // 二次方程 Ke*d² + Kl*d + (Kc - target) = 0，取正根
        const float a = Ke;
        const float b = Kl;
        const float c = Kc - target;
        const float disc = b * b - 4.0f * a * c;
        if (disc >= 0.0f) {
            const float root = (-b + std::sqrt(disc)) / (2.0f * a);
            if (root > 0.0f) {
                return root;
            }
        }
    } else if (std::fabs(Kl) > 1e-6f) {
        // 纯线性衰减：d = (target - Kc) / Kl
        const float d = (target - Kc) / Kl;
        if (d > 0.0f) {
            return d;
        }
    }
    // 无衰减（Kc=1, Kl=0, Ke=0）或方程无正根：影响无限远，
    // 回退到地形平面尺寸的一半作为经验影响范围
    return 100.0f;
}

/*
 * 构造"本地 +Z 轴对齐 direction"的右手正交基
 *
 * 以 direction 为本地 Z 轴（zAxis），用"上向量"叉积构造右手正交基
 * xAxis/yAxis/zAxis，满足 x = up × z、y = z × x。三轴作为旋转矩阵的列向量时，
 * 等价于把本地 +Z 旋转对齐到 direction（与旧 Model::SetGizmoTransform 同构）。
 */
void DebugDraw::BuildBasis(const glm::vec3 &direction,
                           glm::vec3 &xAxis, glm::vec3 &yAxis, glm::vec3 &zAxis) {
    // 归一化目标方向；零向量兜底指向 -Z，避免后续叉积退化
    zAxis = glm::normalize(direction);
    if (glm::length(zAxis) < 1e-6f) {
        zAxis = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    // 选取不与 zAxis 平行的"上向量"，避免叉积结果为零向量
    const glm::vec3 up = (std::fabs(zAxis.y) > 0.999f)
                             ? glm::vec3(1.0f, 0.0f, 0.0f)
                             : glm::vec3(0.0f, 1.0f, 0.0f);

    // 构造右手正交基：x = up × z，y = z × x，三者互相垂直且满足右手系
    xAxis = glm::normalize(glm::cross(up, zAxis));
    yAxis = glm::cross(zAxis, xAxis);
}