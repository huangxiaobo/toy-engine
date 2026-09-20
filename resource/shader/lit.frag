/*
 * lit.frag —— 标准管线 ③ 顶点颜色 x 材质 x 光照（Blinn-Phong + 阴影 + 法线贴图）
 *
 * 渲染特性（业界 Blinn-Phong 标准光照模型）：
 *   1. albedo 合成：albedo = 顶点颜色 x 漫反射贴图 x 材质漫反射色
 *   2. 法线贴图：存在时用 TBN 把切空间法线变换到世界空间，替代几何法线
 *   3. 多光源累加：方向光（支持阴影）+ 点光源数组 + 聚光灯数组
 *   4. 阴影：方向光阴影贴图采样（3×3 PCF 软阴影 + 斜率 bias）
 *   5. 镜面反射：Blinn-Phong（半程向量 H），Shininess 仅作高光聚敛指数
 *
 * uniform 命名与 engine/technique/technique_light.cpp 的上传代码严格对应，
 * 不得随意改名，否则光照数据无法写入（C++ 侧 GetUniformLocation 返回 -1 被忽略）。
 */
#version 330 core

uniform vec3 gViewPos;   // 摄像机世界空间位置

// ---- 方向光（单实例）----
// 强度语义：AmbientIntensity/DiffuseIntensity/SpecularIntensity 为独立的权重系数，
// 与各自颜色（AmbientColor 等）线性相乘后累加到最终光照。
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

// ---- 点光源（最多 8 个，数组索引与 C++ 端 gPointLights[i] 对应）----
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
uniform int   gPointLightNum;   // 本帧激活的点光源数量

// ---- 聚光灯（最多 8 个）----
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
    float Cutoff;       // 内锥角（度）：全亮
    float OuterCutoff;  // 外锥角（度）：衰减到 0
};
uniform SpotLight gSpotLights[8];
uniform int  gSpotLightNum;    // 本帧激活的聚光灯数量

// ---- 材质（TechniqueLight::SetMaterial 上传，见 technique_light.cpp）----
struct Material {
    vec3  AmbientColor;   // 环境反射系数
    vec3  DiffuseColor;   // 漫反射系数
    vec3  SpecularColor;  // 镜面反射系数
    float Shininess;      // 高光聚敛指数（pow 幂次，绝不乘入颜色）
};
uniform Material gMaterial;

// ---- 纹理（Mesh::Draw 绑定，见 mesh.cpp）----
uniform sampler2D gTexture;    // 漫反射贴图，纹理单元 0
uniform sampler2D gNormalMap;  // 法线贴图，纹理单元 1
uniform int gHasTexture   = 0; // 是否绑定了漫反射贴图
uniform int gHasNormalMap = 0; // 是否绑定了法线贴图

// ---- 阴影（Mesh::Draw / Technique::SetShadowMap 上传）----
uniform sampler2D shadowMap;   // 深度贴图，纹理单元 2
uniform int gUseShadow = 0;    // 本帧是否启用阴影采样（0/1）

// 阴影 bias 缩放系数（默认 1.0）：正交阴影范围自适应后 bias 相对比例变化，面板滑块现场微调
uniform float gShadowBiasScale = 1.0;

in VS_OUT {
    vec3  Color0;
    vec2  TexCoords;
    vec3  WorldPos0;
    vec3  Normal0;
    vec3  Tangent0;
    vec3  Bitangent0;
    vec4  FragPosLightSpace;
} v2f;

out vec4 color;

/*
 * 阴影判定：把片元在光源裁剪空间的坐标变换到 [0,1]，与深度贴图比较
 *
 * 返回 1.0 表示片元处于阴影中。bias 采用"基础值 + 坡度相关"补偿，
 * 抵消自阴影痤疮（表面片元深度与贴图深度数值相同造成的自身遮挡伪影）：
 *   - baseBias 补偿深度贴图量化误差
 *   - slopeFactor 项：表面越倾斜（与光照方向夹角越大）误差越大，需更大偏置
 *
 * 采样方式为 3×3 PCF：对周围 9 个贴图像素逐一比较后求平均，
 * 让阴影边缘从单探针的硬边变成软过渡，消除锯齿化/像素化。
 * （贴图边界色为 1.0，越界样本按"被照亮"处理，不会误判阴影）
 */
float ShadowCalculation(vec4 fragPosLightSpace, vec3 N) {
    // 透视除法转 NDC，再映射到 [0,1] 纹理坐标
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;

    // 反自阴影偏置：随表面与光照方向的夹角增大而增大
    float bias = max(0.005 + 0.01 * (1.0 - dot(N, normalize(gDirectionLight.Direction))), 0.002) * gShadowBiasScale;

    // 3×3 PCF：逐像素比较阴影，结果取 9 个样本的平均（软阴影）
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // 超出贴图范围（NDC 越界）的片元当作为被照亮，避免边界采样到 CLAMP 值误判全黑
    if (projCoords.z > 1.0) {
        shadow = 0.0;
    }
    return shadow;
}

/*
 * 单光源 Blinn-Phong 光照（对方向光/点光/聚光统一调用）
 *
 * 返回该光源的环境+漫反射+镜面反射贡献（仅颜色量，不含衰减/锥角处理），
 * 由调用方（CalcDirectionLight/CalcPointLight/CalcSpotLight）乘上衰减后累加，
 * 避免"全局累计后再整体除衰减"污染先前光源的贡献。
 *
 * 参数语义：
 *   LightDirection           —— 光照方向（从光源指向片元，函数内归一化）
 *   Shadow                   —— 阴影系数（方向光传入实际阴影值，其余光源传 0）
 *   AmbientIntensity 等      —— 环境/漫反射/镜面权重，与颜色线性相乘
 *
 * 阴影只衰减漫反射/镜面，环境光不受遮挡影响。
 */
vec4 CalcLightInternal(vec3 LightColor, vec3 LightDirection, vec3 N,
                       float AmbientIntensity, float DiffuseIntensity, float SpecularIntensity,
                       float Shadow) {
    vec3 L = normalize(LightDirection);

    // 环境光：LightColor x 材质环境色 x 权重（不受遮挡影响）
    vec4 result = vec4(LightColor, 1.0) * vec4(gMaterial.AmbientColor, 1.0) * AmbientIntensity;

    // 漫反射：Lambert 余弦定律
    // 注意：这里不再乘 gMaterial.DiffuseColor —— 材质漫反射色已在 main() 的 albedo
    // 合成中乘入（albedo *= gMaterial.DiffuseColor），此处若再乘会导致漫反射色被
    // 平方、非线性（旧版历史缺陷）。光照函数只负责亮度/角度/阴影因子。
    float diff = max(dot(N, -L), 0.0);
    if (diff > 0.0) {
        vec3 diffuse = LightColor * DiffuseIntensity * diff * (1.0 - Shadow);
        result += vec4(diffuse, 0.0);

        // 镜面反射：Blinn-Phong 用半程向量 H 代替反射向量 R，视觉更精准且性能更好
        vec3 V = normalize(gViewPos - v2f.WorldPos0);   // 观察方向
        vec3 H = normalize(L + V);                       // 半程向量
        float spec = pow(max(dot(N, H), 0.0), gMaterial.Shininess);
        if (spec > 0.0) {
            vec3 specular = LightColor * gMaterial.SpecularColor * SpecularIntensity * spec * (1.0 - Shadow);
            result += vec4(specular, 0.0);
        }
    }
    return result;
}

vec4 CalcDirectionLight(vec3 N, float Shadow) {
    return CalcLightInternal(gDirectionLight.Color, gDirectionLight.Direction, N,
                             gDirectionLight.AmbientIntensity, gDirectionLight.DiffuseIntensity,
                             gDirectionLight.SpecularIntensity, Shadow);
}

/*
 * 点光源：与方向光相同的光照核，贡献按距离物理衰减
 * （点光源结构体无 SpecularIntensity 字段，镜面权重固定 1.0，由颜色亮度天然控制强弱）
 */
vec4 CalcPointLight(int idx, vec3 N) {
    vec3  L    = v2f.WorldPos0 - gPointLights[idx].Position;
    float dist = length(L);
    L = normalize(L);

    vec4 contrib = CalcLightInternal(gPointLights[idx].Color, L, N,
                                     gPointLights[idx].AmbientIntensity,
                                     gPointLights[idx].DiffuseIntensity, 1.0, 0.0);
    // 物理衰减：1 / (Kc + Kl*d + Ke*d^2)
    float atten = gPointLights[idx].AttenuationConstant
                + gPointLights[idx].AttenuationLinear * dist
                + gPointLights[idx].AttenuationExp * dist * dist;
    return contrib / atten;
}

/*
 * 聚光灯：点光源 + 内锥/外锥软边缘
 *   外锥之外无光；内锥内全亮；内锥到外锥线性衰减（平滑过渡）
 */
vec4 CalcSpotLight(int idx, vec3 N) {
    vec3  L          = normalize(v2f.WorldPos0 - gSpotLights[idx].Position);
    float spotFactor = dot(L, normalize(gSpotLights[idx].Direction));

    float cosCutoff      = cos(radians(gSpotLights[idx].Cutoff));
    float cosOuterCutoff = cos(radians(gSpotLights[idx].OuterCutoff));

    // 外锥之外完全无光
    if (spotFactor <= cosOuterCutoff) {
        return vec4(0.0);
    }

    float dist = length(v2f.WorldPos0 - gSpotLights[idx].Position);

    vec4 contrib = CalcLightInternal(gSpotLights[idx].Color, L, N,
                                     gSpotLights[idx].AmbientIntensity,
                                     gSpotLights[idx].DiffuseIntensity,
                                     gSpotLights[idx].SpecularIntensity, 0.0);
    // 距离衰减 + 内锥->外锥线性平滑过渡
    float atten = gSpotLights[idx].AttenuationConstant
                + gSpotLights[idx].AttenuationLinear * dist
                + gSpotLights[idx].AttenuationExp * dist * dist;
    float smoothFactor = clamp((spotFactor - cosOuterCutoff) / (cosCutoff - cosOuterCutoff), 0.0, 1.0);
    return contrib / atten * smoothFactor;
}

void main() {
    // ---- 1. 法线：优先使用法线贴图（切空间扰动），否则用几何法线 ----
    vec3 N;
    if (gHasNormalMap == 1) {
        // TBN：切线、副切线、法线构成切空间正交基（行向量，对应纹理 U/V/W 轴）
        mat3 TBN = mat3(normalize(v2f.Tangent0), normalize(v2f.Bitangent0), normalize(v2f.Normal0));
        // 法线贴图颜色解码：RGB -> [-1, 1] 的切空间法线
        vec3 tangentNormal = texture(gNormalMap, v2f.TexCoords).rgb * 2.0 - 1.0;
        tangentNormal = normalize(tangentNormal);
        // 变换到世界空间并重新归一化
        N = normalize(TBN * tangentNormal);
    } else {
        N = normalize(v2f.Normal0);
    }

    // ---- 2. albedo 合成：顶点颜色 x 漫反射贴图 x 材质漫反射系数 ----
    vec3 albedo = v2f.Color0;
    if (gHasTexture == 1) {
        // 漫反射贴图按 sRGB 编码存储，采样后先解码到线性空间再参与光照：
        // 否则贴图被当作线性值乘进 albedo，到最后 gamma 编码时会被提亮、发灰
        vec3 texColor = texture(gTexture, v2f.TexCoords).rgb;
        texColor = pow(texColor, vec3(2.2));
        albedo *= texColor;
    }
    albedo *= gMaterial.DiffuseColor;

    // ---- 3. 光照累加 ----
    vec4 total = vec4(0.0);

    // 方向光阴影：仅当本帧存在有效深度贴图时计算（gUseShadow = 1）
    float shadow = (gUseShadow == 1) ? ShadowCalculation(v2f.FragPosLightSpace, N) : 0.0;
    total += CalcDirectionLight(N, shadow);

    // 点光源 / 聚光灯：各自衰减/锥角处理后累加贡献
    for (int i = 0; i < gPointLightNum; i++) {
        total += CalcPointLight(i, N);
    }
    for (int i = 0; i < gSpotLightNum; i++) {
        total += CalcSpotLight(i, N);
    }

    // ---- 4. 合成：albedo x 光照 ----
    color = vec4(total.rgb * albedo, 1.0);
}