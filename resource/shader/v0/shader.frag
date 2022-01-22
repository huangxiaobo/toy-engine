#version 330

struct Attenuation
{
    float Constant;
    float Linear;
    float Exp;
};

struct Light {
    vec3    Color;
    vec3    Position;

    float   AmbientIntensity;
    float   DiffuseIntensity;
    vec3    DiffuseColor;
    vec3    SpecularColor;
    Attenuation Atten;
};

uniform Light gLight[1];

// 材质结构体
struct Material{
    vec3 AmbientColor;//环境
    vec3 DiffuseColor;//漫反射
    vec3 SpecularColor;//镜面反射
    float Shininess;//镜面反射光泽
};

uniform Material gMaterial;

in vec3 WorldPos0;
in vec3 Normal0;

out vec4 color;
void main()
{
    // 计算顶点在世界坐标系的位置
    vec3 P = WorldPos0;
    // 将法线向量转化到直接坐标系
    vec3 N = normalize(Normal0);
    // 计算光源到顶点的距离
    vec3 L = gLight[0].Position.xyz - P.xyz;

    // 计算散射
    vec3 diffuse = max(dot(N, L), 0.0) * gMaterial.DiffuseColor * gLight[0].Color;
    color = vec4(diffuse, 1.0);
}