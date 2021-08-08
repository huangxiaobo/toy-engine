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

    vec4 AmbientColor = vec4(0, 0, 0, 0);
    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);


    vec3 P = WorldPos0.xyz;
    vec3 N = normalize(Normal0);

    // 计算光源到顶点的距离
    vec3 L = gLight[0].Position.xyz - WorldPos0.xyz;

    //
    vec3 LightDirection = WorldPos0.xyz - gLight[0].Position.xyz;
    float LightDistance = length(LightDirection);
    LightDirection = normalize(LightDirection);


    // Ambient
    AmbientColor.xyz = gLight[0].Color * gMaterial.AmbientColor;

    float DiffuseFactor = dot(N, -LightDirection);

    if (DiffuseFactor > 0) {
        // 漫反射光照
        DiffuseColor = vec4(gLight[0].Color, 1.0f) * gLight[0].DiffuseIntensity * DiffuseFactor;

        // 计算眼睛观察方向
        vec3 VertexToEye = normalize(gViewPos - WorldPos0);
        // 计算反射光方向
        vec3 LightReflect = normalize(reflect(LightDirection, N));
        // 计算反射光与观测方向的夹角
        float SpecularFactor = dot(VertexToEye, LightReflect);
        // 计算镜面反射强度
        if (SpecularFactor > 0) {
            SpecularFactor = pow(SpecularFactor, gMaterial.Shininess);
            SpecularColor = vec4(gLight[0].Color * gMaterial.Shininess * SpecularFactor, 1.0f);
        }
    }
    color = vec4(AmbientColor.xyz + DiffuseColor.xyz + SpecularColor.xyz, 1.0);
}