#ifndef __LIGHT_H__
#define __LIGHT_H__

#include <glm/glm.hpp>


class Model;

enum LightType
{
    LightTypePoint,     // 点光源
    LightTypeDirection, // 方向光源
    LightTypeSpot       // 聚光灯
};

class Light
{
public:
    Light();

    Light(const LightType &light_type) : light_type(light_type) {}
    virtual const LightType &GetLightType() const { return light_type; };

    virtual ~Light() {}

private:
    LightType light_type;
};

struct DirectionLight : public Light
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

struct PointLight : public Light
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
    PointLight() : Light(LightTypePoint) {}
    ~PointLight() {}
};

#endif // __LIGHT_H__