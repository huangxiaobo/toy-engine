#version 330 core

/*
 * 天空穹 - 顶点着色器
 *
 * 功能：
 *   1. 将半球体顶点变换到裁剪空间
 *   2. 强制深度为 1.0（远裁剪面），确保天空穹始终在所有物体之后
 *   3. 传递法线方向到片段着色器，用于计算渐变
 *
 * 关键技术：
 *   gl_Position.z = gl_Position.w 会使裁剪空间 z/w = 1.0
 *   即 NDC 深度 = 1.0（最远处），这样天空穹永远不会遮挡场景物体
 */

// --- 顶点属性 ---
layout (location = 0) in vec3 position;   // 半球体顶点位置
layout (location = 1) in vec3 normal;     // 法线方向（与位置方向相同）

// --- Uniform 矩阵 ---
uniform mat4 projection;  // 投影矩阵
uniform mat4 view;        // 视图矩阵
uniform mat4 model;       // 模型矩阵（仅摄像机位置平移）

// --- 输出到片段着色器 ---
out vec3 vNormal;  // 世界空间法线方向，用于渐变计算

void main() {
    // MVP 变换
    gl_Position = projection * view * model * vec4(position, 1.0);

    // 强制深度为 1.0（远裁剪面）
    // gl_Position.z/w = 1.0 → NDC 深度 = 1.0 → 最远位置
    gl_Position.z = gl_Position.w;

    // 传递法线（模型矩阵仅平移，不影响方向）
    vNormal = normal;
}
