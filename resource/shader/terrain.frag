#version 330 core

uniform vec3 gViewPos;

// 方向光结构体
struct DirectionLight {
    vec3    Direction;
    vec3    Color;
    float   AmbientIntensity;
    float   DiffuseIntensity;
    float   SpecularIntensity;
    vec3    AmbientColor;
    vec3    DiffuseColor;
    vec3    SpecularColor;
};

uniform DirectionLight gDirectionLight;

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

// 聚光灯结构体
struct SpotLight {
    vec3    Position;
    vec3    Direction;
    vec3    Color;

    float   AmbientIntensity;
    float   DiffuseIntensity;
    float   SpecularIntensity;
    vec3    AmbientColor;
    vec3    DiffuseColor;
    vec3    SpecularColor;

    float   AttenuationConstant;
    float   AttenuationLinear;
    float   AttenuationExp;

    float   Cutoff;
    float   OuterCutoff;
};

uniform SpotLight gSpotLights[8];
uniform int gSpotLightNum;

// 材质结构体
struct Material{
    vec3 AmbientColor;//环境
    vec3 DiffuseColor;//漫反射
    vec3 SpecularColor;//镜面反射
    float Shininess;//镜面反射光泽
};

uniform Material gMaterial;

uniform sampler2D groundTexture;

in VsOut {
    vec3 Color0;
    vec2 TexCoords;
    vec3 WorldPos0;
    vec3 Normal0;
} v2f;

out vec4 color;

vec4 CalcLightInternal(vec3 LightColor, vec3 LightDirection, vec3 Normal) {
    vec4 AmbientColor = vec4(LightColor, 1.0f) * vec4(gMaterial.AmbientColor, 1.0) * 0.2;
    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0) {
        // 漫反射光照
        DiffuseColor = vec4(LightColor * gMaterial.DiffuseColor * DiffuseFactor, 1.0f);

        // 计算眼睛观察方向
        vec3 VertexToEye = normalize(gViewPos - v2f.WorldPos0);
        // 计算反射光方向
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        // 计算反射光与观测方向的夹角
        float SpecularFactor = dot(VertexToEye, LightReflect);
        // 计算镜面反射强度
        if (SpecularFactor > 0) {
            SpecularFactor = pow(SpecularFactor, gMaterial.Shininess);
            SpecularColor = vec4(LightColor * gMaterial.SpecularColor * gMaterial.Shininess * SpecularFactor, 1.0f);
        }
    }

    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcDirectionLight(vec3 Normal) {
    return CalcLightInternal(gDirectionLight.Color, gDirectionLight.Direction, Normal);
}

vec4 CalcPointLight(int Index, vec3 Normal)
{
    vec3 LightDirection = v2f.WorldPos0 - gPointLights[Index].Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    vec4 Color = CalcLightInternal(gPointLights[Index].Color, LightDirection, Normal);
    float Attenuation = gPointLights[Index].AttenuationConstant + gPointLights[Index].AttenuationLinear * Distance + gPointLights[Index].AttenuationExp * Distance * Distance;

    return Color / Attenuation;
}

vec4 CalcSpotLight(int Index, vec3 Normal) {
    vec3 LightToPixel = normalize(v2f.WorldPos0 - gSpotLights[Index].Position);
    float SpotFactor = dot(LightToPixel, normalize(gSpotLights[Index].Direction));

    float CosCutoff = cos(radians(gSpotLights[Index].Cutoff));
    float CosOuterCutoff = cos(radians(gSpotLights[Index].OuterCutoff));

    float Distance = length(v2f.WorldPos0 - gSpotLights[Index].Position);
    float Attenuation = gSpotLights[Index].AttenuationConstant + gSpotLights[Index].AttenuationLinear * Distance + gSpotLights[Index].AttenuationExp * Distance * Distance;

    vec4 Color = CalcLightInternal(gSpotLights[Index].Color, LightToPixel, Normal) / Attenuation;

    if (SpotFactor <= CosOuterCutoff) {
        return vec4(0, 0, 0, 0);
    }

    float SmoothFactor = clamp((SpotFactor - CosOuterCutoff) / (CosCutoff - CosOuterCutoff), 0.0f, 1.0f);
    return Color * SmoothFactor;
}

void main() {
    vec3 N = normalize(v2f.Normal0);
    
    // 采样纹理
    vec4 texColor = texture(groundTexture, v2f.TexCoords);

    // 计算方向光
    vec4 totalColor = CalcDirectionLight(N);

    // 累加多个点光源
    for (int i = 0; i < gPointLightNum; i++) {
        totalColor += CalcPointLight(i, N);
    }

    // 累加多个聚光灯
    for (int i = 0; i < gSpotLightNum; i++) {
        totalColor += CalcSpotLight(i, N);
    }
    
    // 将光照颜色与纹理颜色相乘
    color = texColor * vec4(totalColor.rgb, 1.0);
}
