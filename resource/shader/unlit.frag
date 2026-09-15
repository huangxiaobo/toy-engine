/*
 * unlit.frag —— 标准管线 ① 纯顶点颜色（无光照）
 *
 * 接收顶点着色器透传的颜色并直接输出，无任何着色逻辑。
 * 对应 unlit.vert，两者必须成对使用。
 */
#version 330 core

in VS_OUT {
    vec3 Color0;   // 顶点颜色（来自顶点着色器）
} v2f;

// 片元输出统一使用 location 0（Shader::BindFragDataLocation 绑定名字 "color"）
out vec4 color;

void main() {
    color = vec4(v2f.Color0, 1.0);
}