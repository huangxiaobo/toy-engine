#ifndef __LIGHT_H__
#define __LIGHT_H__

#include <glm/glm.hpp>
#include <memory>
#include <string>

class Model;

enum LightType
{
    LightTypeNone,      // 未定义
    LightTypePoint,     // 点光源
    LightTypeDirection, // 方向光源
    LightTypeSpot       // 聚光灯
};

const std::string LightTypeNameNone = "未定义";
const std::string LightTypeNamePoint = "点光源";
const std::string LightTypeNameDirection = "方向光源";
const std::string LightTypeNameSpot = "聚光灯";

class Light
{
public:
    Light();

    Light(std::string name, const LightType &light_type);
    virtual const LightType &GetLightType() const { return m_light_type; };
    virtual const std::string GetLightTypeName() const;

    std::string GetName() const;

    virtual Model *GetModel() = 0;
    virtual ~Light() {}

private:
    LightType m_light_type;
    std::string m_name;
};

class DirectionLight : public Light
{
public:
    glm::vec3 Direction;
    glm::vec3 Color;

    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;

    float AmbientIntensity;
    float DiffuseIntensity;
    float SpecularIntensity;
};

// Atten参数参考表
// Distance	Constant    Linear    Quadratic
// 200	    1.0	        0.022      0.0019
// 325	    1.0	        0.014      0.0007
// 600	    1.0	        0.007      0.0002
class PointLight : public Light
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

    Model *m_model;

public:
    PointLight(std::string name);
    ~PointLight();

    Model *GetModel() override { return m_model; }
};

#endif // __LIGHT_H__