/*
 * unlit.vert —— 标准管线 ① 纯顶点颜色（无光照）
 *
 * 渲染特性：仅输出顶点颜色，不计算任何光照/材质/纹理，
 * 用于调试几何形状、展示模型原始顶点色，或作为手绘风格的最底层基色。
 *
 * 顶点属性布局（与 engine/mesh/mesh.h 的 Vertex 布局一致）：
 *   location 0 = position    世界/模型空间顶点位置
 *   location 1 = vertcolor   顶点颜色（无光照时直接作为输出颜色）
 *
 * 矩阵 uniform：
 *   projection / view / model —— 依次变换：模型空间 -> 世界 -> 观察 -> 裁剪
 */
#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;

// 顶点->片元接口块：纯颜色管线只需透传顶点色
out VS_OUT {
    vec3 Color0;   // 顶点颜色
} v2f;

void main() {
    v2f.Color0 = vertcolor;
    gl_Position = projection * view * model * vec4(position, 1.0);
}