#version 330 core

/*
 * 后处理片元着色器：场景 HDR 纹理 → 默认帧缓冲
 *
 * toneMapMode 控制两种输出方式：
 *   1（默认）：Reinhard tone mapping + gamma 校正。
 *     HDR 线性值经 hdr/(hdr+1) 压缩到 [0,1) 避免高光溢出，
 *     再 pow(1/2.2) 做 gamma 校正——显示器按 sRGB 解码后呈现正确亮度。
 *   0：直通输出（调试用），scene 纹理原样拷贝，便于对比 tone mapping 效果。
 */

in vec2 v2f_uv;

uniform sampler2D sceneTex;
uniform int toneMapMode = 1; // 1=Reinhard+gamma，0=直通

out vec4 FragColor;

void main() {
    vec3 hdr = texture(sceneTex, v2f_uv).rgb;

    vec3 mapped = hdr;
    if (toneMapMode == 1) {
        mapped = hdr / (hdr + vec3(1.0));       // Reinhard：线性值压缩到 [0,1)
        mapped = pow(mapped, vec3(1.0 / 2.2));  // gamma 校正（sRGB 编码）
    }

    FragColor = vec4(mapped, 1.0);
}