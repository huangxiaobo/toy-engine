/*
 * toon.vert —— 标准管线 ④ 卡通渲染（顶点着色器）
 *
 * 与 lit.vert 相同的顶点变换与世界空间输出集合，但不需要 TBN
 * （卡通渲染以风格化光照为主，通常不使用法线贴图扰动）。
 *
 * 顶点属性布局：
 *   location 0 = position    顶点位置
 *   location 1 = vertcolor   顶点颜色
 *   location 2 = normal      顶点法线
 *   location 3 = texcoords   纹理坐标（可选卡通纹理/色块贴图）
 *
 * 矩阵 uniform：projection / view / model / lightSpace（阴影）
 */
#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpace;   // 光源空间矩阵（阴影坐标）

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texcoords;

out VS_OUT {
    vec3  Color0;            // 顶点颜色
    vec2  TexCoords;         // 纹理坐标
    vec3  WorldPos0;         // 世界空间位置（光照/视向量/边缘光计算用）
    vec3  Normal0;           // 世界空间法线
    vec4  FragPosLightSpace; // 光源裁剪空间位置（阴影）
} v2f;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);

    v2f.Color0    = vertcolor;
    v2f.TexCoords = texcoords;

    vec4 worldPos = model * vec4(position, 1.0);
    v2f.WorldPos0 = worldPos.xyz;

    // 法线矩阵：非均匀缩放下法线用模型矩阵的逆转置变换
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    v2f.Normal0 = normalize(normalMatrix * normal);

    // 光源空间坐标：基于乘过 model 的世界坐标，与深度 Pass 变换一致
    v2f.FragPosLightSpace = lightSpace * worldPos;
}