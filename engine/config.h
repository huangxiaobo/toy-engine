#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>

#include "camera/camera.h" // 摄像机投影枚举（CameraConfig::Projection 使用）

class WindowConfig {
public:
    int WindowWidth;
    int WindowHeight;
};

class ClipConfig {
public:
    float ClipNear;
    float ClipFar;
    float ClipFov;
    float ClipAspect;
};

class CameraConfig {
public:
    std::string Name;  // 摄像机名称
    glm::vec3 Position;
    glm::vec3 Target;
    glm::vec3 Up;
    ProjectionType Projection = ProjectionType::Perspective; // 投影模式（perspective 透视 / orthographic 正交，默认透视）
};

class PointLightConfig {
public:
    std::string Name;
    std::string Id;
    bool Enabled = true;
    glm::vec3 Position;
    glm::vec3 Color;

    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;

    // 三通道光照强度（world.yaml ambient/diffuse/specular 的 intensity 字段），默认 1.0
    float AmbientIntensity = 1.0f;
    float DiffuseIntensity = 1.0f;
    float SpecularIntensity = 1.0f;

    struct {
        float Constant;
        float Linear;
        float Exp;
    } Attenuation;
};

class DirectionLightConfig {
public:
    std::string Name;
    std::string Id;
    bool Enabled = true;
    glm::vec3 Direction;
    glm::vec3 Color;

    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;

    // 三通道光照强度（world.yaml ambient/diffuse/specular 的 intensity 字段），默认 1.0
    float AmbientIntensity = 1.0f;
    float DiffuseIntensity = 1.0f;
    float SpecularIntensity = 1.0f;
};

class SpotLightConfig {
public:
    std::string Name;
    std::string Id;
    bool Enabled = true;
    glm::vec3 Position;
    glm::vec3 Direction;
    glm::vec3 Color;

    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;

    // 三通道光照强度（world.yaml ambient/diffuse/specular 的 intensity 字段），默认 1.0
    float AmbientIntensity = 1.0f;
    float DiffuseIntensity = 1.0f;
    float SpecularIntensity = 1.0f;

    struct {
        float Constant;
        float Linear;
        float Exp;
    } Attenuation;

    float Cutoff;
    float OuterCutoff;
};

/*
 * 材质配置（MaterialConfig）
 *
 * 对应 world.yaml 中 model 的 material 段。为支持「用 MTL 实现材质」，
 * 材质不再内联在 yaml 中，而是通过 MTL 文件路径 + 材质名引用：
 *
 *   material:
 *     file: "./resource/model/sphere/sphere.mtl"  # MTL 文件路径
 *     name: "sphere_material"                     # newmtl 声明的材质名
 *
 * 渲染器使用 MtlParser 从 MTL 文件加载出 Material 对象。
 */
class MaterialConfig {
public:
    std::string File; // MTL 文件路径
    std::string Name; // 材质名称（对应 newmtl <name>）
};

class ParticleConfig {
public:
    std::string Name;
    std::string Id;
    glm::vec3 Position;
    
    float EmitRate;
    int MaxParticles;
    
    float MinLife, MaxLife;
    float MinSize, MaxSize;
    glm::vec3 MinVelocity, MaxVelocity;
    float MinSizeEnd, MaxSizeEnd;
    
    glm::vec3 Gravity;
    float Drag;
};

class MeshConfig {
public:
    std::string Name;
    std::string File;
};

class SkyDomeConfig {
public:
    float Radius = 500.0f;
    int Sectors = 32;
    int Stacks = 16;
    glm::vec3 HorizonColor = glm::vec3(0.6f, 0.7f, 0.9f);
    glm::vec3 ZenithColor = glm::vec3(0.1f, 0.2f, 0.5f);
    // 地面雾色（下半球），默认取地平线色 0.6 倍（暗化大气色）
    glm::vec3 GroundColor = glm::vec3(0.36f, 0.42f, 0.54f);
};

/*
 * 地形配置（TerrainConfig）
 *
 * 对应 world.yaml 中的 terrain 段，用于配置程序化网格地形（无 LOD）。
 * 这些参数会被解析后传给 TerrainManager（engine/terrain/terrain_manager.h）。
 *
 * 各字段含义：
 *   - planeSize:     地形平面世界尺寸（长 = 宽，以原点为中心）
 *   - resolution:    网格分辨率（每边格子数，顶点数 = resolution + 1）
 *   - heightScale:   地形最大高度（噪声高度缩放因子，0 = 平坦平面）
 *   - noiseSeed:     噪声生成器的随机种子，相同种子产生相同地形
 */
class TerrainConfigCfg {
public:
    float PlaneSize = 100.0f;
    int Resolution = 128;
    float HeightScale = 20.0f;
    unsigned int NoiseSeed = 12345;
};

class ModelConfig {
public:
    std::string Name;
    std::string Effect;
    std::string ShaderVertFile;
    std::string ShaderFragFile;

    glm::vec3 Position;
    // 三轴欧拉角（度）：x/y/z 分量分别对应绕 X/Y/Z 轴，旋转顺序 Y → X → Z
    glm::vec3 Rotation;
    glm::vec3 Scale;

    MeshConfig Mesh;

    MaterialConfig Material;
};

/*
 * 动画配置（AnimationConfig）
 *
 * 对应 world.yaml 的 animations 段，绑定一个已存在的模型或灯光做动画。
 * 各通道（位移/缩放/旋转/颜色/强度）各自独立可选，未配置的通道保持对象原值：
 *
 *   - translate: 正弦振荡 position = center + amplitude * sin(2π·frequency·t)
 *   - scale:     正弦振荡 scale      = base + amplitude * sin(2π·frequency·t)
 *   - rotate:    非 spin 时正弦摆动；spin 模式下匀速旋转（speed 各轴度/秒）
 *   - color:     灯光颜色正弦振荡（R,G,B 振幅）
 *   - intensity: 灯光强度正弦振荡（振幅取 vec3.x，仅灯光可动画）
 *
 * ModelName/LightName 为引用的目标名（world.yaml models[].name / lights[].name），
 * 由渲染器查找绑定；两字段最多填一个。
 */
class AnimationConfig {
public:
    std::string Name;
    std::string ModelName;   // 绑定的模型名（与 LightName 二选一）
    std::string LightName;   // 绑定的灯光名（与 ModelName 二选一）
    bool Enabled = true;

    // ---- 位移通道（可选）----
    bool HasTranslate = false;
    glm::vec3 TranslateCenter{};     // 振荡中心（世界位置）
    glm::vec3 TranslateAmplitude{};  // 单侧振幅
    float TranslateFrequency = 1.0f; // 频率（Hz）

    // ---- 圆周轨道通道（可选）----
    // 绕 Y 轴画圈：x/z 分量 90° 相位差（cos/sin），y 固定为轨道中心高度
    // 点光源无朝向，"绕 Y 轴旋转"即此语义（ambient 同级字段，与 translate 二选一）
    bool HasOrbit = false;
    glm::vec3 OrbitCenter{};         // 轨道中心（世界位置）
    glm::vec3 OrbitRadius{};         // 轨道半径（x/z 分量生效）
    float OrbitFrequency = 1.0f;     // 角速度（圈/秒）

    // ---- 缩放通道（可选）----
    bool HasScale = false;
    glm::vec3 ScaleBase{};           // 缩放中心
    glm::vec3 ScaleAmplitude{};      // 单侧振幅
    float ScaleFrequency = 1.0f;     // 频率（Hz）

    // ---- 旋转通道（可选）----
    bool HasRotate = false;
    bool RotateSpin = false;         // true = 匀速旋转（用 speed），false = 正弦摆动
    glm::vec3 RotateBase{};          // 旋转基准（欧拉角，度）
    glm::vec3 RotateAmplitude{};     // 摆动振幅（欧拉角，度）
    float RotateFrequency = 1.0f;    // 摆动频率（Hz）
    glm::vec3 RotateSpeed{};         // 匀速旋转角速度（度/秒）

    // ---- 灯光颜色通道（可选）----
    bool HasColor = false;
    glm::vec3 ColorCenter{};         // 颜色振荡中心（R,G,B）
    glm::vec3 ColorAmplitude{};      // 各通道单侧振幅
    float ColorFrequency = 1.0f;     // 频率（Hz）

    // ---- 灯光强度通道（可选）----
    bool HasIntensity = false;
    glm::vec3 IntensityCenter{};     // 强度中心（取 x 分量）
    glm::vec3 IntensityAmplitude{};  // 单侧振幅（取 x 分量）
    float IntensityFrequency = 1.0f; // 频率（Hz）
};

class Config {
public:
    Config();

    ~Config();

    static Config *LoadFromYaml(const std::string &filename);

    WindowConfig Window;
    glm::vec4 ClearColor;
    ClipConfig Clip;
    std::vector<CameraConfig> Cameras;  // 支持多个摄像机
    std::vector<PointLightConfig> PointLights;
    std::vector<DirectionLightConfig> DirectionLights;
    std::vector<SpotLightConfig> SpotLights;
    std::vector<ParticleConfig> Particles;
    SkyDomeConfig SkyDome;
    std::vector<ModelConfig> Models;
    std::vector<AnimationConfig> Animations;

    // 地形配置（程序化LOD地形）
    TerrainConfigCfg Terrain;
};

#endif // __CONFIG_H__
