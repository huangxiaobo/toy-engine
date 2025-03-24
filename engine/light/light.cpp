#include "light.h"
Light::Light()
{
    m_light_type = LightTypeNone;
}

Light::Light(std::string name, const LightType &light_type)
    : m_name(name), m_light_type(light_type)
{
}

const std::string Light::GetLightTypeName() const
{
    switch (m_light_type)
    {
    case LightTypeDirection:
        return LightTypeNameDirection;
    case LightTypePoint:
        return LightTypeNamePoint;
    case LightTypeSpot:
        return LightTypeNameSpot;
    default:
        return LightTypeNameNone;
    }
    return std::string();
}

std::string Light::GetName() const
{
    return m_name;
}

PointLight::PointLight(std::string name) : Light(name, LightTypePoint)
{
}

PointLight::~PointLight()
{
}
