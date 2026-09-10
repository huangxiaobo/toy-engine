#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpace; // 光源空间矩阵（阴影映射）：把世界坐标变换到光源视角

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoords;

out VsOut {
    vec3 Color0;
    vec2 TexCoords;
    vec3 WorldPos0;
    vec3 Normal0;
    vec4 FragPosLightSpace; // 顶点在光源裁剪空间的位置（阴影判定用）
} v2f;

void main() {
    gl_Position = projection * view * model * vec4(position, 1);
    
    v2f.Color0 = vertcolor;
    v2f.TexCoords = texCoords;
    
    // 计算世界空间位置
    vec4 position_h = vec4(position, 1.0);
    v2f.WorldPos0 = (model * position_h).xyz;
    
    // 转换法线到世界空间
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    v2f.Normal0 = normalize(normalMatrix * normal);

    // 计算顶点在光源空间的位置，供片元着色器采样阴影贴图。
    // 必须用世界坐标 WorldPos0（已乘 model），与深度 Pass 的 gDepthMVP = lightSpace * model 一致，
    // 否则带缩放/平移的模型阴影坐标会与深度贴图错位，导致阴影完全无法显示。
    v2f.FragPosLightSpace = lightSpace * vec4(v2f.WorldPos0, 1.0);
}
