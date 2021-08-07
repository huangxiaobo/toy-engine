#version 330

uniform vec3 gViewPos;

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

// N = the surface normal vector
// L = a vector from the surface to the light source

void main() {

    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);
    float gMatSpecularIntensity = 100;


    vec3 Normal = normalize(Normal0);

    // 计算光源到顶点的距离
    vec3 L = gLight[0].Position.xyz - WorldPos0.xyz;

    // g
    vec3 LightDirection = normalize(gLight[0].Position.xyz - WorldPos0.xyz);

    // 计算散射 漫反射
    vec3 diffuse = max(dot(Normal, L), 0.0) * gLight[0].DiffuseColor * gMaterial.DiffuseColor;

    float DiffuseFactor = dot(Normal, LightDirection);
    if (DiffuseFactor > 0) {
        vec3 VertexToEye =normalize(gViewPos.xyz - WorldPos0.xyz);
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        float SpecularFactor = dot(VertexToEye, LightReflect);
        if (SpecularFactor > 0) {
            SpecularFactor = pow(SpecularFactor, gMaterial.Shininess);
            SpecularColor = vec4(gMaterial.SpecularColor * gMaterial.Shininess * SpecularFactor, 1.0f);
        } else {
            SpecularColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    } else {
        SpecularColor = vec4(0.0, 0.0, 1.0, 1.0);
    }
    color = vec4(diffuse + gMaterial.SpecularColor, 1.0);
}