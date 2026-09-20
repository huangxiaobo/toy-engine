#version 330 core

/*
 * 后处理片元着色器：场景 HDR 纹理 → 默认帧缓冲
 *
 * toneMapMode 控制三种输出方式：
 *   1（默认）：ACES filmic tone mapping + gamma 校正。
 *     ACES 近似曲线（Narkowicz 版）比 Reinhard 对比度更高、高光不漂白、
 *     色彩饱和度保持更好，是当前影视/游戏常用的映射曲线。
 *   2：Reinhard tone mapping + gamma（旧曲线，保留用于对比）。
 *   0：直通输出（调试用），scene 纹理原样拷贝，便于对比 tone mapping 效果。
 *
 * exposure：曝光系数，先乘到 HDR 线性值再映射（>1 提亮，<1 压暗）。
 *
 * saturation / contrast：tonemap+gamma 之后统一调整影调（默认 1.0 不调整），
 * 饱和度按亮度回退（mix），对比度围绕 0.5 中点缩放。
 */

in vec2 v2f_uv;

uniform sampler2D sceneTex;
uniform int toneMapMode = 1;   // 1=ACES+gamma，2=Reinhard+gamma，0=直通
uniform float exposure = 1.0;  // 曝光系数
uniform float saturation = 1.0; // 饱和度（1.0 = 原图）
uniform float contrast = 1.0;   // 对比度（1.0 = 原图）

out vec4 FragColor;

// ACES filmic 近似（Krzysztof Narkowicz 版），输入输出均为 [0,∞) HDR → [0,1]
vec3 ACESFilm(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    // 曝光调整先于 tone mapping：直接在 HDR 线性空间缩放
    vec3 hdr = texture(sceneTex, v2f_uv).rgb * exposure;

    vec3 mapped = hdr;
    if (toneMapMode == 1) {
        mapped = ACESFilm(hdr);
        mapped = pow(mapped, vec3(1.0 / 2.2));   // gamma 校正（sRGB 编码）
    } else if (toneMapMode == 2) {
        mapped = hdr / (hdr + vec3(1.0));        // Reinhard：线性值压缩到 [0,1)
        mapped = pow(mapped, vec3(1.0 / 2.2));
    }

    // 色彩调节（ACES 输出后按观感微调）：饱和度为 0 时整体退化为灰度（亮度不变），
    // 对比度围绕 0.5 中点缩放，<1 压平影调 / >1 拉开明暗
    if (saturation != 1.0 || contrast != 1.0) {
        float luma = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
        mapped = mix(vec3(luma), mapped, saturation);
        mapped = (mapped - 0.5) * contrast + 0.5;
    }

    FragColor = vec4(mapped, 1.0);
}