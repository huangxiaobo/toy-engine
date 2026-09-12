#version 330 core

/*
 * DebugDraw 调试着色器 - 顶点阶段
 *
 * 纯顶点色输出：顶点位置经 projection * view 变换（顶点已处于世界空间，
 * 由 CPU 端 DebugDraw 直接生成，不再需要 model 矩阵），颜色原样传给片段阶段。
 * 不参与光照计算，用于光源 gizmo / 调试线框等可视化几何。
 */

uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;

out VsOut {
    vec3 Color0;
} v2f;

void main() {
    v2f.Color0 = vertcolor;
    gl_Position = projection * view * vec4(position, 1);
}