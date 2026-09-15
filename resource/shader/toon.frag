/*
 * toon.frag —— 标准管线 ④ 卡通渲染（Toon Shading）
 *
 * 渲染特性（业界经典卡通光照模型，山口丰式三色调风格扩展版）：
 *   1. 漫反射色阶化：Lambert NdotL 量化到 uToonBands 个离散台阶，
 *      并用 fwidth 对台阶边缘做抗锯齿过渡，形成"硬边色块"的卡通明暗
 *   2. 卡通高光：Blinn 半程向量与法线夹角超过阈值才出现白色亮斑（两级阈值）
 *   3. 边缘光（Rim）：法线与视线近垂直（物体轮廓）处叠加边缘光，
 *      模拟卡通描边的轮廓受光效果，让角色从背景中分离
 *   4. 完整多光源 + 阴影：方向光（阴影）/ 点光源 / 聚光灯全部支持
 *
 * 卡通参数通过 uniform 默认值提供（GLSL 330 允许 uniform 初始化默认值），
 * C++ 侧可用 SetUniform 在运行时覆盖（详见 mainwindow 调试面板可扩展点）。
 *
 * uniform 命名与 technique_light.cpp 上传代码严格对应，不得随意改名。
 */
#version 330 core

uniform vec3 gViewPos;

// ---- 光源结构体：命名与 lit.frag / C++ 上传代码完全一致 ----
struct DirectionLight {
    vec3  Direction;
    vec3  Color;
    float AmbientIntensity;
    float DiffuseIntensity;
    float SpecularIntensity;
    vec3  AmbientColor;
    vec3  DiffuseColor;
    vec3  SpecularColor;
};
uniform DirectionLight gDirectionLight;

struct PointLight {
    vec3  Color;
    vec3  Position;
    float AmbientIntensity;
    float DiffuseIntensity;
    vec3  DiffuseColor;
    vec3  SpecularColor;
    float AttenuationConstant;
    float AttenuationLinear;
    float AttenuationExp;
};
uniform PointLight gPointLights[8];
uniform int gPointLightNum;

struct SpotLight {
    vec3  Position;
    vec3  Direction;
    vec3  Color;
    float AmbientIntensity;
    float DiffuseIntensity;
    float SpecularIntensity;
    vec3  AmbientColor;
    vec3  DiffuseColor;
    vec3  SpecularColor;
    float AttenuationConstant;
    float AttenuationLinear;
    float AttenuationExp;
    float Cutoff;
    float OuterCutoff;
};
uniform SpotLight gSpotLights[8];
uniform int gSpotLightNum;

// ---- 材质 ----
struct Material {
    vec3  AmbientColor;
    vec3  DiffuseColor;
    vec3  SpecularColor;
    float Shininess;
};
uniform Material gMaterial;

// ---- 纹理（可选卡通纹理/色块）----
uniform sampler2D gTexture;
uniform int gHasTexture = 0;

// ---- 阴影 ----
uniform sampler2D shadowMap;
uniform int gUseShadow = 0;

// =================== 卡通风格参数（默认值可被 C++ 覆盖） ===================
uniform int   uToonBands      = 3;    // 漫反射色阶数：3 = 经典三色调卡通（亮/中/暗）
uniform float uSpecularHard   = 0.45; // 高光亮斑一级阈值（超过才出现亮斑）
uniform float uSpecularSoft   = 0.25; // 高光亮斑二级阈值（两阈值之间为中间调亮斑）
uniform float uRimStrength    = 0.55; // 边缘光强度
uniform float uRimPower       = 4.0;  // 边缘光指数：越大轮廓光带越窄

in VS_OUT {
    vec3  Color0;
    vec2  TexCoords;
    vec3  WorldPos0;
    vec3  Normal0;
    vec4  FragPosLightSpace;
} v2f;

out vec4 color;

/*
 * 阴影判定（与 lit.frag 相同的方向光阴影采样）
 */
float ShadowCalculation(vec4 fragPosLightSpace, vec3 N) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // 斜率相关 bias 抵消自阴影痤疮
    float bias = max(0.005 + 0.01 * (1.0 - dot(N, normalize(gDirectionLight.Direction))), 0.002);
    float shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
    if (projCoords.z > 1.0) {
        shadow = 0.0;
    }
    return shadow;
}

/*
 * 卡通光照核（单光源）：返回该光源贡献
 *
 * 与 lit 管线的关键差异：
 *   - 漫反射：NdotL 经 floor 量化到 uToonBands 个台阶（亮部/中间调/暗部），
 *     fwidth 抗锯齿平滑台阶边缘，形成卡通硬边色块
 *   - 镜面：两级阈值 step 判定亮斑等级（uSpecularSoft 与 uSpecularHard），
 *     高光呈离散亮斑而非连续衰减
 */
vec4 CalcToonLightInternal(vec3 LightColor, vec3 LightDirection, vec3 N,
                           float AmbientIntensity, float DiffuseIntensity,
                           float SpecularIntensity, float Shadow) {
    vec3 L = normalize(LightDirection);

    // 环境光：不受阴影与色阶影响
    vec4 result = vec4(LightColor, 1.0) * vec4(gMaterial.AmbientColor, 1.0) * AmbientIntensity;

    float diff = max(dot(N, -L), 0.0);
    if (diff > 0.0) {
        // ---- 漫反射色阶化（卡通核心）----
        // diff * uToonBands 的整数部分决定落在哪个色阶；
        // fract 用于对"跨阶边缘"做 fwidth 平滑过渡，消除锯齿
        float scaled = diff * float(uToonBands);
        float band   = floor(scaled) / float(uToonBands);
        // 台阶边缘抗锯齿：fwidth 传播因子与当前 step 位置，混入下一级色阶
        float edge = smoothstep(0.0, fwidth(scaled), fract(scaled));
        band += edge / float(uToonBands);

        // 注意：这里不再乘 gMaterial.DiffuseColor —— 材质漫反射色已在 main() 的
        // albedo 合成中乘入（albedo *= gMaterial.DiffuseColor），此处再乘会导致
        // 漫反射色被平方、非线性（与 lit.frag 同一历史缺陷，一并修复）。
        vec3 diffuse = LightColor * DiffuseIntensity * band * (1.0 - Shadow);
        result += vec4(diffuse, 0.0);

        // ---- 卡通高光：离散两级亮斑 ----
        vec3 V = normalize(gViewPos - v2f.WorldPos0);
        vec3 H = normalize(L + V);   // Blinn 半程向量
        float s = max(dot(N, H), 0.0);
        // step：硬边界 0/1。两级叠加得到三档高光：暗(hard 未命中)/中(soft 命中)/亮(两级都命中)
        float specLevel = step(uSpecularSoft, s) + step(uSpecularHard, s);
        vec3 specular = LightColor * gMaterial.SpecularColor * SpecularIntensity
                      * specLevel * (1.0 - Shadow);
        result += vec4(specular, 0.0);
    }
    return result;
}

vec4 CalcToonDirectionLight(vec3 N, float Shadow) {
    return CalcToonLightInternal(gDirectionLight.Color, gDirectionLight.Direction, N,
                                 gDirectionLight.AmbientIntensity, gDirectionLight.DiffuseIntensity,
                                 gDirectionLight.SpecularIntensity, Shadow);
}

vec4 CalcToonPointLight(int idx, vec3 N) {
    vec3  L    = v2f.WorldPos0 - gPointLights[idx].Position;
    float dist = length(L);
    L = normalize(L);

    vec4 contrib = CalcToonLightInternal(gPointLights[idx].Color, L, N,
                                         gPointLights[idx].AmbientIntensity,
                                         gPointLights[idx].DiffuseIntensity, 1.0, 0.0);
    float atten = gPointLights[idx].AttenuationConstant
                + gPointLights[idx].AttenuationLinear * dist
                + gPointLights[idx].AttenuationExp * dist * dist;
    return contrib / atten;
}

vec4 CalcToonSpotLight(int idx, vec3 N) {
    vec3  L          = normalize(v2f.WorldPos0 - gSpotLights[idx].Position);
    float spotFactor = dot(L, normalize(gSpotLights[idx].Direction));

    float cosCutoff      = cos(radians(gSpotLights[idx].Cutoff));
    float cosOuterCutoff = cos(radians(gSpotLights[idx].OuterCutoff));

    if (spotFactor <= cosOuterCutoff) {
        return vec4(0.0);
    }

    float dist = length(v2f.WorldPos0 - gSpotLights[idx].Position);

    vec4 contrib = CalcToonLightInternal(gSpotLights[idx].Color, L, N,
                                         gSpotLights[idx].AmbientIntensity,
                                         gSpotLights[idx].DiffuseIntensity,
                                         gSpotLights[idx].SpecularIntensity, 0.0);
    float atten = gSpotLights[idx].AttenuationConstant
                + gSpotLights[idx].AttenuationLinear * dist
                + gSpotLights[idx].AttenuationExp * dist * dist;
    float smoothFactor = clamp((spotFactor - cosOuterCutoff) / (cosCutoff - cosOuterCutoff), 0.0, 1.0);
    return contrib / atten * smoothFactor;
}

void main() {
    vec3 N = normalize(v2f.Normal0);

    // ---- albedo：顶点颜色 x 可选漫反射贴图（无贴图时退化为顶点色） ----
    vec3 albedo = v2f.Color0;
    if (gHasTexture == 1) {
        albedo *= texture(gTexture, v2f.TexCoords).rgb;
    }
    albedo *= gMaterial.DiffuseColor;

    // ---- 多光源卡通光照累加 ----
    vec3 lighting = vec3(0.0);

    // 方向光 + 阴影
    float shadow = (gUseShadow == 1) ? ShadowCalculation(v2f.FragPosLightSpace, N) : 0.0;
    lighting += CalcToonDirectionLight(N, shadow).rgb;

    // 点光源 / 聚光灯
    for (int i = 0; i < gPointLightNum; i++) {
        lighting += CalcToonPointLight(i, N).rgb;
    }
    for (int i = 0; i < gSpotLightNum; i++) {
        lighting += CalcToonSpotLight(i, N).rgb;
    }

    // ---- 边缘光（Rim）：法线与视线近垂直处（模型轮廓）叠加菲涅尔边缘光 ----
    vec3  V   = normalize(gViewPos - v2f.WorldPos0);
    float rim = pow(1.0 - max(dot(N, V), 0.0), uRimPower) * uRimStrength;
    lighting += vec3(rim);

    // ---- 合成：albedo x 卡通光照 ----
    color = vec4(albedo * lighting, 1.0);
}