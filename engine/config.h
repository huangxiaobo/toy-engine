#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>

class WindowConfig
{
public:
    int WindowWidth;
    int WindowHeight;
};

class ClipConfig
{
public:
    float ClipNear;
    float ClipFar;
    float ClipFov;
    float ClipAspect;
};

class CameraConfig
{
public:
    glm::vec3 Position;
    glm::vec3 Target;
    glm::vec3 Up;
};

class PointLightConfig
{
public:
    glm::vec3 Position;
    glm::vec3 Color;

    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;

    float AmbientIntensity;
    float DiffuseIntensity;
    float SpecularIntensity;

    struct
    {
        float Constant;
        float Linear;
        float Exp;
    } Attenuation;
};

class MaterialConfig
{
public:
    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;
    float Shininess;
};

class MeshConfig
{
public:
    std::string Name;
    std::string File;
};
class ModelCoinfig
{
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

class Config
{

public:
    Config();
    ~Config();

public:
    static Config *LoadFromXml(std::string xml_file);

public:
    WindowConfig Window;
    glm::vec4 ClearColor;
    ClipConfig Clip;
    CameraConfig Camera;
    std::vector<PointLightConfig> PointLights;
    std::vector<ModelCoinfig> Models;
};

#endif // __CONFIG_H__