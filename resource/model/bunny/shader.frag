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

in VsOut {
    vec3 WorldPos0;
    vec3 Normal0;
} v2f;

out vec4 color;

/*
 * 统一的 Blinn-Phong 单光源光照计算（与 terrain.frag 保持一致的强度语义）
 *
 * 参数语义：
 *   - AmbientIntensity：环境光权重，公式 LightColor * 材质Ambient * AmbientIntensity
 *   - DiffuseIntensity：漫反射权重，公式 LightColor * 材质Diffuse * DiffuseIntensity * dot(N, -L)
 *   - SpecularIntensity：镜面权重，公式 LightColor * 材质Specular * SpecularIntensity * pow(dot, Shininess)
 *
 * 注意：gMaterial.Shininess 只作高光聚敛指数（pow 的幂次），绝不能再乘到颜色上。
 */
vec4 CalcLightInternal(vec3 LightColor, vec3 LightDirection, vec3 Normal,
                       float AmbientIntensity, float DiffuseIntensity, float SpecularIntensity) {
    // 归一化光照方向（方向光的 Direction 来自 world.yaml 且未在 C++ 侧归一化，必须在此处理）
    LightDirection = normalize(LightDirection);

    // 环境光：与光源颜色、材质环境色、环境光强度三者线性相关
    vec4 AmbientColor = vec4(LightColor, 1.0f) * vec4(gMaterial.AmbientColor, 1.0) * AmbientIntensity;
    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0) {
        // 漫反射光照：颜色 * 强度 * 角度因子
        DiffuseColor = vec4(LightColor * gMaterial.DiffuseColor * DiffuseIntensity * DiffuseFactor, 1.0f);

        // 计算眼睛观察方向
        vec3 VertexToEye = normalize(gViewPos - v2f.WorldPos0);
        // 计算反射光方向
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        // 计算反射光与观测方向的夹角
        float SpecularFactor = dot(VertexToEye, LightReflect);
        // 计算镜面反射强度（Shininess 仅作聚敛指数，不再乘入颜色）
        if (SpecularFactor > 0) {
            SpecularFactor = pow(SpecularFactor, gMaterial.Shininess);
            SpecularColor = vec4(LightColor * gMaterial.SpecularColor * SpecularIntensity * SpecularFactor, 1.0f);
        }
    }

    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcDirectionLight(vec3 Normal) {
    // 方向光：三个强度分量全部来自配置，可独立调节
    return CalcLightInternal(gDirectionLight.Color, gDirectionLight.Direction, Normal,
                             gDirectionLight.AmbientIntensity, gDirectionLight.DiffuseIntensity, gDirectionLight.SpecularIntensity);
}

vec4 CalcPointLight(int Index, vec3 Normal)
{
    vec3 LightDirection = v2f.WorldPos0 - gPointLights[Index].Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    // 点光源结构体没有 SpecularIntensity 字段，镜面强度固定取 1.0，由其颜色亮度天然控制镜面强弱
    vec4 Color = CalcLightInternal(gPointLights[Index].Color, LightDirection, Normal,
                                   gPointLights[Index].AmbientIntensity, gPointLights[Index].DiffuseIntensity, 1.0f);
    float Attenuation = gPointLights[Index].AttenuationConstant + gPointLights[Index].AttenuationLinear * Distance + gPointLights[Index].AttenuationExp * Distance * Distance;

    return Color * (1.0f / Attenuation);
}

vec4 CalcSpotLight(int Index, vec3 Normal) {
    vec3 LightToPixel = normalize(v2f.WorldPos0 - gSpotLights[Index].Position);
    // 计算片段相对光源方向与聚光灯朝向的夹角
    float SpotFactor = dot(LightToPixel, normalize(gSpotLights[Index].Direction));

    // 内锥角余弦值（全亮）
    float CosCutoff = cos(radians(gSpotLights[Index].Cutoff));
    // 外锥角余弦值（衰减到 0）
    float CosOuterCutoff = cos(radians(gSpotLights[Index].OuterCutoff));

    // 计算距离衰减
    float Distance = length(v2f.WorldPos0 - gSpotLights[Index].Position);
    float Attenuation = gSpotLights[Index].AttenuationConstant + gSpotLights[Index].AttenuationLinear * Distance + gSpotLights[Index].AttenuationExp * Distance * Distance;

    // 聚光灯：三个强度全量传入
    vec4 Color = vec4(0, 0, 0, 0);
    Color += CalcLightInternal(gSpotLights[Index].Color, LightToPixel, Normal,
                               gSpotLights[Index].AmbientIntensity, gSpotLights[Index].DiffuseIntensity, gSpotLights[Index].SpecularIntensity) * (1.0f / Attenuation);

    // 在外锥角之外直接丢弃
    if (SpotFactor <= CosOuterCutoff) {
        return vec4(0, 0, 0, 0);
    }

    // 平滑过渡：内锥角内全亮，内锥到外锥之间线性衰减
    float SmoothFactor = clamp((SpotFactor - CosOuterCutoff) / (CosCutoff - CosOuterCutoff), 0.0f, 1.0f);

    return Color * SmoothFactor;
}

void main() {
    vec3 N = normalize(v2f.Normal0);

    // 计算方向光（如果有配置）
    vec4 totalColor = CalcDirectionLight(N);

    // 累加多个点光源
    for (int i = 0; i < gPointLightNum; i++) {
        totalColor += CalcPointLight(i, N);
    }

    // 累加多个聚光灯
    for (int i = 0; i < gSpotLightNum; i++) {
        totalColor += CalcSpotLight(i, N);
    }

    color = vec4(totalColor.rgb, 1.0);
}