#ifndef __DEBUG_DRAW_H__
#define __DEBUG_DRAW_H__

#include <glad/gl.h> // 必须在所有库的顶部（AGENTS.md 教训5）

#include <cstddef> // offsetof
#include <glm/glm.hpp>
#include <vector>

// 前向声明：Draw*Light 仅以指针参数引用光源，完整定义在 .cpp 中引入
class PointLight;
class SpotLight;
class DirectionLight;
class Technique;

/*
 * DebugDraw：独立于 Model/Mesh 体系的光源位置/范围可视化渲染器（方案 B）
 *
 * 与旧的"每光源挂一个 Model 线框网格"方案相比，本类把调试可视化从模型管线
 * 中解耦出来，作为一个独立的调试绘制系统：
 *
 *   1. 逐帧收集：各模块（目前是光源可视化）把线段顶点（世界坐标 + 颜色）
 *      追加进 CPU 端顶点列表，不涉及任何 Model/Mesh/Texture 资源。
 *   2. 统一提交：Render() 一次性把收集到的顶点上传到动态 VBO，以
 *      glDrawArrays(GL_LINES) 批量绘制，避免每光源一次 draw call。
 *   3. 无状态同步：灯的衰减参数、位置、朝向变化后，下一帧绘制时直接从
 *      Light 对象读取，不存在"先创建后绑定"的同步问题（对比 AGENTS.md 教训1）。
 *
 * 生命周期约定：
 *   - 着色器（Technique）由调用方（Renderer）创建并共享，本类只引用不拥有；
 *   - VAO/VBO/CPU 顶点列表由本类自管，析构时释放。
 */
class DebugDraw {
public:
    DebugDraw();

    ~DebugDraw();

    /*
     * 初始化：创建 VAO/VBO 顶点布局，并绑定调用方共享的调试着色器
     *
     * effect 生命周期归调用方管理（Renderer 统一释放），本类只持有裸指针。
     */
    void Init(Technique *effect);

    // ---- 基础调试原语（世界空间坐标，供任意调试用途扩展）----

    // 绘制一条线段：from -> to
    void DrawLine(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color);

    // 绘制线框球体：中心 center、半径 radius，用 X/Y/Z 三个正交平面圆环勾勒
    void DrawSphereWireframe(const glm::vec3 &center, float radius, const glm::vec3 &color, int segments = 16);

    // 绘制线框圆锥：锥顶 apex、轴线方向 direction、半角 angleDeg、锥长 length
    void DrawConeWireframe(const glm::vec3 &apex, const glm::vec3 &direction, float angleDeg,
                           float length, const glm::vec3 &color, int segments = 12);

    // 绘制带箭头头的线段：主轴线 + 顶端两条斜线箭头
    void DrawArrow(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color);

    // ---- 光源可视化 gizmo（参数每帧直接从 Light 对象读取，自动跟随编辑）----

    // 点光源：斐波那契球面放射线，长度 = 由衰减系数反推的影响半径
    void DrawPointLight(const PointLight *light);

    // 聚光灯：双层双锥线框（实线内锥 = Cutoff 全亮范围 + 虚线外锥 = OuterCutoff 半影范围 + 中轴线）
    void DrawSpotLight(const SpotLight *light);

    // 方向光：场景原点三轴十字标记 + 沿光照方向延伸的方向线
    void DrawDirectionLight(const DirectionLight *light);

    /*
     * 每帧统一提交渲染：
     *   - 把 CPU 端收集的线段顶点全量上传到动态 VBO（GL_DYNAMIC_DRAW）；
     *   - 用调试着色器（纯顶点色，忽略光照）以 GL_LINES 一次性批量绘制；
     *   - 绘制结束后自动 Clear()，释放本帧顶点列表。
     * 调用时机由渲染器决定（光源 gizmo 应置于场景 Pass 内、地形/模型绘制之后）。
     */
    void Render(const glm::mat4 &projection, const glm::mat4 &view);

    // 清空本帧收集的顶点（Render 尾部自动调用；也可用于手动提前清空）
    void Clear();

private:
    /*
     * 由点光源衰减系数反推"影响边界"半径
     *
     * 衰减公式 att = 1 / (Kc + Kl*d + Ke*d²)，求衰减到阈值 threshold 处的距离 d，
     * 作为点光源 gizmo 放射线的影响半径，保证球体轮廓严格对应真实光照衰减范围。
     */
    static float ComputePointLightRadius(const PointLight *light, float threshold = 0.6f);

    /*
     * 构造"本地 +Z 轴对齐 direction"的右手正交基
     *
     * 与旧 Model::SetGizmoTransform 相同的旋转语义：以 direction 为 zAxis，
     * 用"上向量"叉积构造 xAxis/yAxis，三轴互垂且满足右手系。
     * 用于在 CPU 端直接把本地线框几何变换到世界空间。
     */
    static void BuildBasis(const glm::vec3 &direction, glm::vec3 &xAxis, glm::vec3 &yAxis, glm::vec3 &zAxis);

private:
    // 调试顶点格式：位置 + 颜色，对应 debug.vert 的 location 0 / 1（轻量，不含法线/UV）
    struct DebugVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    Technique *m_effect = nullptr;        // 调试着色器（裸指针，只引用不拥有）
    GLuint m_vao = 0;                     // 顶点数组对象
    GLuint m_vbo = 0;                     // 动态顶点缓冲（每帧全量重传）
    std::vector<DebugVertex> m_vertices;  // 逐帧收集的线段顶点，Render 后清空
};

#endif // __DEBUG_DRAW_H__