#version 330 core

/*
 * 粒子系统 - 片段着色器
 *
 * 功能：
 *   1. 将点精灵裁剪为圆形（消除方形边缘）
 *   2. 采样纹理并叠加粒子颜色
 *   3. 根据生命周期控制透明度淡出
 *
 * 关键概念：
 *   gl_PointCoord - 点精灵内部的纹理坐标，范围 [0,1]
 *     (0,0) = 左下角, (1,1) = 右上角, (0.5, 0.5) = 中心
 *
 * 算法流程：
 *   1. 计算当前片段到点精灵中心的距离
 *   2. 用 smoothstep 实现边缘抗锯齿的圆形裁剪
 *   3. 采样纹理获取基础颜色和 alpha
 *   4. 乘以生命比例实现淡出效果
 */

// --- 从顶点着色器插值输入 ---
in vec3 vColor;       // 粒子颜色（由 CPU 端根据 MinColor/MaxColor 随机生成）
in float vLifeRatio;  // 生命比例 [0,1]，1=刚出生，0=即将死亡

// --- Uniform ---
uniform sampler2D particleTexture;  // 粒子纹理（默认为棋盘格纹理，提供 alpha 通道）

// --- 输出 ---
out vec4 color;  // 最终片段颜色（RGBA）

void main() {
    // ========== 第一步：圆形裁剪 ==========
    // gl_PointCoord 范围 [0,1]，中心在 (0.5, 0.5)
    // 将坐标原点移到中心，范围变为 [-0.5, 0.5]
    vec2 coord = gl_PointCoord - vec2(0.5);

    // 计算当前片段到中心的距离（欧几里得距离）
    // dist = 0 → 中心, dist = 0.5 → 边缘, dist > 0.5 → 超出圆形范围
    float dist = length(coord);

    // ========== 第二步：抗锯齿边缘 ==========
    // fwidth(dist) = |d(dist)/dx| + |d(dist)/dy|，即距离的屏幕空间导数
    // 用于计算像素级别的过渡宽度，确保在不同分辨率下边缘都平滑
    float edge = fwidth(dist);

    // smoothstep(edge0, edge1, x) 在 [edge0, edge1] 之间平滑插值
    //   dist < 0.5 - edge → alpha = 1.0（完全不透明，圆形内部）
    //   dist > 0.5        → alpha = 0.0（完全透明，圆形外部）
    //   中间区域          → 平滑过渡（抗锯齿）
    float alpha = 1.0 - smoothstep(0.5 - edge, 0.5, dist);

    // 丢弃几乎完全透明的片段，避免不必要的片段着色和深度写入
    if (alpha < 0.01) discard;

    // ========== 第三步：纹理采样 ==========
    // 使用原始 gl_PointCoord（未偏移）采样纹理
    // 纹理提供额外的颜色细节和 alpha 通道
    vec4 texColor = texture(particleTexture, gl_PointCoord);

    // ========== 第四步：生命周期淡出 ==========
    // fade = 纹理 alpha × 生命比例
    //   刚出生时 vLifeRatio ≈ 1.0 → fade ≈ texColor.a（不透明）
    //   死亡时   vLifeRatio ≈ 0.0 → fade ≈ 0.0（完全透明）
    // 实现粒子随生命减少逐渐淡出的效果
    float fade = texColor.a * vLifeRatio;

    // ========== 第五步：最终颜色合成 ==========
    // RGB: 粒子颜色 × 纹理颜色（纹理用于增加细节变化）
    // A:   圆形 alpha × 淡出 alpha（两者的乘积）
    color = vec4(vColor * texColor.rgb, alpha * fade);
}
