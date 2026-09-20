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

// 阴影贴图（深度贴图）：绑定到纹理单元2，存储从光源视角看到的最近深度
uniform sampler2D shadowMap;
// 是否启用阴影采样（Mesh::Draw 每帧同步：阴影可用为1，否则为0）
uniform int gUseShadow;

// 阴影 bias 缩放系数（默认 1.0）：正交阴影范围自适应后 bias 相对比例变化，面板滑块现场微调
uniform float gShadowBiasScale = 1.0;

in VsOut {
    vec3 Color0;
    vec2 TexCoords;
    vec3 WorldPos0;
    vec3 Normal0;
    vec4 FragPosLightSpace; // 顶点在光源裁剪空间的位置（阴影判定用）
} v2f;

out vec4 color;

/*
 * 阴影判定
 *
 * 把片段在光源空间的裁剪坐标变换到 [0,1] 纹理坐标，用片段深度与阴影贴图中
 * 存储的最近深度比较：若片段离光源更远(被遮挡)，则处于阴影中返回 1。
 * bias 用来抵消自阴影痤疮（片元深度贴图深度数值相同造成的自身遮挡伪影）。
 * 采样为 3×3 PCF：逐像素比较后平均，阴影边缘软过渡（贴图边界色 1.0，越界按被照亮）。
 */
float ShadowCalculation(vec4 fragPosLightSpace, vec3 Normal) {
    // 透视除法转为 NDC，再映射到 [0,1] 的纹理坐标范围
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;

    // 阴影深度比较的反自阴影偏置。
    // 根据当前投影参数重新计算：
    //   - 正交范围：±210 单位，深度范围 [1, 100]（99 单位）
    //   - sphere 中心深度 ≈ 0.192，地形深度 ≈ 0.293，深度差 ≈ 0.1
    //   - 阴影贴图分辨率：2048×2048
    //
    // bias 公式：baseBias + slopeFactor * (1.0 - dot(N,L))
    //   - baseBias = 0.005：补偿量化误差和深度比较精度（深度范围99，2048分辨率下每像素约0.00005）
    //   - slopeFactor = 0.01：表面越倾斜（与光源夹角越大），采样误差越大，需要更大 bias
    //   - 对于垂直光照下的平坦地形（dot=1.0），bias = 0.005
    //   - 对于45°斜面（dot≈0.707），bias ≈ 0.008
    //   - 这个范围既能抑制自阴影痤疮，又不会让 sphere 阴影明显"漂移"
    float bias = max(0.005 + 0.01 * (1.0 - dot(Normal, normalize(gDirectionLight.Direction))), 0.002) * gShadowBiasScale;

    // 3×3 PCF：周围 9 个贴图像素比较后平均，得到软阴影
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // 超出阴影贴图范围的片元当作"被照亮"(边界外无遮挡)，避免采样到 CLAMP_TO_BORDER=1.0
    // 后因 compare 恒成立而被整片误判为阴影
    return shadow;
}

/*
 * 统一的 Blinn-Phong 单光源光照计算
 *
 * 参数语义（手动光源：AmbientIntensity / DiffuseIntensity / SpecularIntensity）：
 *   - AmbientIntensity：环境光权重，公式 LightColor * 材质Ambient * AmbientIntensity
 *   - DiffuseIntensity：漫反射权重，公式 LightColor * 材质Diffuse * DiffuseIntensity * dot(N, -L)
 *   - SpecularIntensity：镜面权重，公式 LightColor * 材质Specular * SpecularIntensity * pow(dot, Shininess)
 *   - Shadow：阴影系数，1.0 完全遮蔽（漫反射/镜面反射乘 (1.0 - Shadow)，环境光不受阴影影响）
 *
 * 注意：gMaterial.Shininess 只作高光聚敛指数（pow 的幂次），绝不能再乘到颜色上，
 * 否则高光强度会被错误放大到快门级数值，这是历史遗留 bug 的修复点。
 */
vec4 CalcLightInternal(vec3 LightColor, vec3 LightDirection, vec3 Normal,
                       float AmbientIntensity, float DiffuseIntensity, float SpecularIntensity,
                       float Shadow) {
    // 归一化光照方向（从光源指向片段的单位向量）。
    // 【重要】方向光的 gDirectionLight.Direction 来自 world.yaml 且未在上传前归一化
    // （technique_light.cpp 直接上传原始值），只有恰好为单位向量时 dot 结果才正确。
    // 这里统一归一化，保证任意配置下漫反射/镜面反射强度不受方向向量长度影响，
    // 也确保下方 reflect() 的入射方向是单位向量。
    LightDirection = normalize(LightDirection);

    // 环境光：与光源颜色、材质环境色、环境光强度三者线性相关，不受阴影遮蔽影响
    vec4 AmbientColor = vec4(LightColor, 1.0f) * vec4(gMaterial.AmbientColor, 1.0) * AmbientIntensity;
    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0) {
        // 漫反射：遮蔽区域阴影系数为1时完全不做漫反射，环境光不受阴影影响
        DiffuseColor = vec4(LightColor * gMaterial.DiffuseColor * DiffuseIntensity * DiffuseFactor * (1.0 - Shadow), 1.0f);

        // 计算眼睛观察方向
        vec3 VertexToEye = normalize(gViewPos - v2f.WorldPos0);
        // 计算反射光方向
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        // 计算反射光与观测方向的夹角
        float SpecularFactor = dot(VertexToEye, LightReflect);
        // 计算镜面反射强度（Shininess 仅作聚敛指数，不再乘入颜色）
        if (SpecularFactor > 0) {
            SpecularFactor = pow(SpecularFactor, gMaterial.Shininess);
            SpecularColor = vec4(LightColor * gMaterial.SpecularColor * SpecularIntensity * SpecularFactor * (1.0 - Shadow), 1.0f);
        }
    }

    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcDirectionLight(vec3 Normal, float Shadow) {
    // 方向光：三个强度分量全部来自配置，可独立调节
    return CalcLightInternal(gDirectionLight.Color, gDirectionLight.Direction, Normal,
                             gDirectionLight.AmbientIntensity, gDirectionLight.DiffuseIntensity, gDirectionLight.SpecularIntensity, Shadow);
}

vec4 CalcPointLight(int Index, vec3 Normal)
{
    vec3 LightDirection = v2f.WorldPos0 - gPointLights[Index].Position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    // 点光源结构体没有 SpecularIntensity 字段，镜面强度固定取 1.0，由其颜色亮度天然控制镜面强弱；
    // 点光源暂不参与阴影，Shadow 传 0.0
    vec4 Color = CalcLightInternal(gPointLights[Index].Color, LightDirection, Normal,
                                   gPointLights[Index].AmbientIntensity, gPointLights[Index].DiffuseIntensity, 1.0, 0.0);
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

    // 聚光灯：三个强度全量传入；暂不参与阴影，Shadow 传 0.0
    vec4 Color = CalcLightInternal(gSpotLights[Index].Color, LightToPixel, Normal,
                                   gSpotLights[Index].AmbientIntensity, gSpotLights[Index].DiffuseIntensity, gSpotLights[Index].SpecularIntensity, 0.0) / Attenuation;

    if (SpotFactor <= CosOuterCutoff) {
        return vec4(0, 0, 0, 0);
    }

    float SmoothFactor = clamp((SpotFactor - CosOuterCutoff) / (CosCutoff - CosOuterCutoff), 0.0f, 1.0f);
    return Color * SmoothFactor;
}

void main() {
    // 地面漫反射采样：world.yaml 中 terrain.texture 若给出有效路径则为真实纹理，
    // 否则替换为棋盘格纹理（两色交替，便于观察地形平面铺展）。
    // 贴图为 sRGB 编码，采样后解码到线性空间参与光照（避免整体发灰）
    vec4 groundColor = texture(groundTexture, v2f.TexCoords);
    groundColor.rgb = pow(groundColor.rgb, vec3(2.2));

    // 结合法线与光照计算最终颜色
    vec4 Color = vec4(0, 0, 0, 0);

    // 仅在启用阴影时计算阴影系数（剖面函数已含自阴影 bias 抵消）
    float Shadow = (gUseShadow == 1) ? ShadowCalculation(v2f.FragPosLightSpace, v2f.Normal0) : 0.0;

    // 遍历所有已启用的点光源（暂未启用，保留供后续开启）
    for (int i = 0; i < gPointLightNum; i++) {
        Color += CalcPointLight(i, v2f.Normal0);
    }

    // 叠加所有聚光灯（暂未启用，保留供后续开启）
    for (int i = 0; i < gSpotLightNum; i++) {
        Color += CalcSpotLight(i, v2f.Normal0);
    }

    // 方向光（阴影只作用于方向光）
    Color += CalcDirectionLight(v2f.Normal0, Shadow);

    // 漫反射颜色由纹理控制（地面着色），叠加光照后的 Lambert 明暗，
    // 使地形的起伏在光照下产生明暗对比，而整体颜色仍来自地面纹理。
    // 阴影已在 CalcDirectionLight 内部按 (1.0 - Shadow) 调暗漫反射/镜面，此处不再重复处理
    color = groundColor * Color;
}
