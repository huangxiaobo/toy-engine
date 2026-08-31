#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>

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
};

class PointLightConfig {
public:
    glm::vec3 Position;
    glm::vec3 Color;

    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;

    float AmbientIntensity;
    float DiffuseIntensity;
    float SpecularIntensity;

    struct {
        float Constant;
        float Linear;
        float Exp;
    } Attenuation;
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
    glm::vec3 MinColor, MaxColor;
    glm::vec3 MinColorEnd, MaxColorEnd;
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
};

/*
 * 地形配置（TerrainConfig）
 *
 * 对应 world.yaml 中的 terrain 段，用于配置程序化 LOD 地形。
 * 这些参数会被解析后传给 TerrainManager（engine/terrain/terrain_manager.h）。
 *
 * 各字段含义：
 *   - chunkSize:       每个 chunk 的世界尺寸（单位）
 *   - baseResolution:  基础网格分辨率（顶点数 = resolution × resolution）
 *   - renderDistance:  渲染距离（chunk 数量），摄像机周围多少圈 chunk 会被加载
 *   - unloadDistance:  卸载距离（chunk 数量），超过该距离的 chunk 被卸载
 *   - heightScale:     地形最大高度（噪声高度缩放因子）
 *   - noiseSeed:       噪声生成器的随机种子，相同种子产生相同地形
 */
class TerrainConfigCfg {
public:
    float ChunkSize = 100.0f;
    int BaseResolution = 64;
    int RenderDistance = 5;
    int UnloadDistance = 7;
    float HeightScale = 20.0f;
    unsigned int NoiseSeed = 12345;
};

class ModelCoinfig {
public:
    std::string Name;
    std::string Effect;
    std::string ShaderVertFile;
    std::string ShaderFragFile;

    glm::vec3 Position;
    glm::f32 Rotation;
    glm::vec3 Scale;

    MeshConfig Mesh;

    MaterialConfig Material;
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
    std::vector<ParticleConfig> Particles;
    SkyDomeConfig SkyDome;
    std::vector<ModelCoinfig> Models;

    // 地形配置（程序化LOD地形）
    TerrainConfigCfg Terrain;
};

#endif // __CONFIG_H__
