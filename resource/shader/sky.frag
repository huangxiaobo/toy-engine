#version 330 core

/*
 * 天空穹 - 片段着色器
 *
 * 功能：
 *   根据法线方向在全球体上生成天空渐变，覆盖 y>0 与 y<0 全方向
 *
 * 渐变算法（h = normalize(vNormal).y，范围 [-1, 1]）：
 *   h < 0（下半球）→ 从地面雾色平滑过渡到地平线色（smoothstep）
 *   h > 0（上半球）→ 从地平线色过渡到天顶色（pow 1.5 幂次曲线）
 *
 * 上下半球分开处理的原因：
 *   旧实现只画上半球、h<0 一律落到地平线色，一旦低头/俯视/正侧视图
 *   地平线以下就露出视口底色。全球体 + 下半球雾色过渡后任何视角都不会漏底色。
 */

// --- 从顶点着色器插值输入 ---
in vec3 vNormal;  // 世界空间法线方向

// --- Uniform ---
uniform vec3 horizonColor;  // 地平线颜色（上半球底部）
uniform vec3 zenithColor;   // 天顶颜色（上半球顶部）
uniform vec3 groundColor;   // 地面雾色（下半球底部），模拟远处大气透视

// --- 输出 ---
out vec4 color;

void main() {
    // 归一化法线，取 y 分量作为渐变因子
    // y = 0 → 地平线；y = ±1 → 天顶/正下方
    float h = normalize(vNormal).y;

    // 下半球：从正下方（groundColor）向地平线平滑过渡，
    // smoothstep 在地平线附近自然衔接，避免色带断层
    vec3 skyColor = mix(groundColor, horizonColor, smoothstep(-0.15, 0.0, h));

    // 上半球：向天顶过渡，沿用幂次曲线（低空贴近地平线色，高空过渡平缓）
    skyColor = mix(skyColor, zenithColor, pow(max(h, 0.0), 1.5));

    // 输出最终颜色（完全不透明）
    color = vec4(skyColor, 1.0);
}