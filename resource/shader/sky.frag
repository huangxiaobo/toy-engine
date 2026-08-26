#version 330 core

/*
 * 天空穹 - 片段着色器
 *
 * 功能：
 *   根据法线方向在两个颜色之间线性插值，生成天空渐变效果
 *
 * 渐变算法：
 *   normalize(vNormal).y 的范围是 [0, 1]
 *     y = 0 → 地平线方向 → 使用 horizonColor
 *     y = 1 → 天顶方向   → 使用 zenithColor
 *   使用 mix() 函数进行线性插值
 */

// --- 从顶点着色器插值输入 ---
in vec3 vNormal;  // 世界空间法线方向

// --- Uniform ---
uniform vec3 horizonColor;  // 地平线颜色（半球底部）
uniform vec3 zenithColor;   // 天顶颜色（半球顶部）

// --- 输出 ---
out vec4 color;

void main() {
    // 归一化法线，取 y 分量作为渐变因子
    // y = 0（水平方向）→ 地平线颜色
    // y = 1（垂直向上）→ 天顶颜色
    float gradientFactor = normalize(vNormal).y;

    // 在地平线颜色和天顶颜色之间线性插值
    vec3 skyColor = mix(horizonColor, zenithColor, gradientFactor);

    // 输出最终颜色（完全不透明）
    color = vec4(skyColor, 1.0);
}
