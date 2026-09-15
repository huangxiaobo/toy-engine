/*
 * textured.vert —— 标准管线 ② 顶点颜色 x 材质贴图（无光照）
 *
 * 渲染特性：顶点颜色与漫反射贴图（gTexture）相乘作为最终颜色，
 * 不计算光照，适合 UI 元素、非光照模型或不依赖场景光源的静态物件。
 *
 * 顶点属性布局（与 engine/mesh/mesh.h 的 Vertex 布局一致）：
 *   location 0 = position    顶点位置
 *   location 1 = vertcolor   顶点颜色（与贴图相乘，可做逐顶点染色）
 *   location 3 = texcoords   纹理坐标（采样 gTexture）
 *
 * 与 unlit 的区别：额外输出 UV 供片元着色器采样漫反射贴图。
 */
#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 3) in vec2 texcoords;

out VS_OUT {
    vec3  Color0;    // 顶点颜色
    vec2  TexCoords; // 纹理坐标
} v2f;

void main() {
    v2f.Color0    = vertcolor;
    v2f.TexCoords = texcoords;
    gl_Position   = projection * view * model * vec4(position, 1.0);
}