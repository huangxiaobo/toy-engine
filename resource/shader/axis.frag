#version 330 core

uniform vec3 gViewPos;

out vec4 FragColor;


in VsOut {
    vec3 Color0;
    vec3 WorldPos0;
    vec3 Normal0;
} v2f;


float saturate(float x) {
    return clamp(x, 0.0f, 1.0f);
}

void main() {

    // 计算眼睛观察方向
    vec3 VertexToEye = normalize(gViewPos - v2f.WorldPos0);
    // 计算法线与观测方向的夹角
    float factor = saturate(dot(VertexToEye, v2f.Normal0));

    FragColor = vec4(v2f.Color0 * factor, 1.0f);
}