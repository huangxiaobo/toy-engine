#version 330 core

/*
 * DebugDraw 调试着色器 - 片段阶段
 *
 * 直接输出顶点色（不透明度 1.0），无任何光照/纹理采样。
 */

in VsOut {
    vec3 Color0;
} v2f;

out vec4 FragColor;

void main() {
    FragColor = vec4(v2f.Color0, 1.0f);
}