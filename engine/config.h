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

class MaterialConfig {
public:
    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;
    float Shininess;
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
};

#endif // __CONFIG_H__
