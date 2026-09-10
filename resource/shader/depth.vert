#version 330 core

/*
 * 深度 Pass 顶点着色器
 *
 * 阴影映射需要一个"只写深度、不写颜色"的额外 Pass：
 * 从光源视角用 gDepthMVP(= lightSpace * model) 变换所有会投射阴影的几何体，
 * 把距离光源的远近写入深度缓冲，供主渲染 Pass 采样判定"该片元是否被遮挡"。
 *
 * 注意：这里不需要任何光照/颜色计算，只需输出裁剪空间位置即可。
 */

// gDepthMVP = lightSpace * model：把模型顶点从模型空间直接变换到光源的裁剪空间
uniform mat4 gDepthMVP;

layout (location = 0) in vec3 position;

void main() {
    gl_Position = gDepthMVP * vec4(position, 1.0);
}
