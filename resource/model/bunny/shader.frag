#version 330 core

uniform vec3 gViewPos;

struct PointLight {
    vec3    Color;
    vec3    Position;

    float   AmbientIntensity;
    float   DiffuseIntensity;
    vec3    DiffuseColor;
    vec3    SpecularColor;
    float   AttenuationConstant;
    float   AttenuationLinear;
    float   AttenuationExp;
};

uniform PointLight gPointLights[8];
uniform int gPointLightNum;

// 材质结构体
struct Material{
    vec3 AmbientColor;//环境
    vec3 DiffuseColor;//漫反射
    vec3 SpecularColor;//镜面反射
    float Shininess;//镜面反射光泽
};

uniform Material gMaterial;

in VsOut {
    vec3 WorldPos0;
    vec3 Normal0;
} v2f;

out vec4 color;

vec4 CalcLightInternal(PointLight Light, vec3 LightDirection, vec3 Normal) {
    vec4 AmbientColor = vec4(Light.Color, 1.0f) * vec4(gMaterial.AmbientColor, 1.0) * Light.AmbientIntensity;
    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0) {
        // 漫反射光照
        DiffuseColor = vec4(Light.Color, 1.0f) * vec4(gMaterial.DiffuseColor, 1.0) * DiffuseFactor;

        // 计算眼睛观察方向
        vec3 VertexToEye = normalize(gViewPos - v2f.WorldPos0);
        // 计算反射光方向
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        // 计算反射光与观测方向的夹角
        float SpecularFactor = dot(VertexToEye, LightReflect);
        // 计算镜面反射强度
        if (SpecularFactor > 0) {
            SpecularFactor = pow(SpecularFactor, gMaterial.Shininess);
            SpecularColor = vec4(Light.Color * gMaterial.SpecularColor * gMaterial.Shininess * SpecularFactor, 1.0f);
        }
    }

    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcPointLight(int Index, vec3 Normal)
{
    vec3 LightDirection = v2f.WorldPos0 - gPointLights[Index].Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    vec4 Color = CalcLightInternal(gPointLights[Index], LightDirection, Normal);
    float Attenuation = gPointLights[Index].AttenuationConstant + gPointLights[Index].AttenuationLinear * Distance + gPointLights[Index].AttenuationExp * Distance * Distance;

    return Color * Attenuation;
}

void main() {
    vec3 N = normalize(v2f.Normal0);

    // 计算多个点光源
    vec4 pointLightColor = vec4(0, 0, 0, 0);
    for (int i = 0; i < gPointLightNum; i++) {
        pointLightColor += CalcPointLight(i, N);
    }
    color = vec4(pointLightColor.rgb, 1.0);
}