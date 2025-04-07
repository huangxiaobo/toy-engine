#include "light.h"
#include "../utils/utils.h"
Light::Light()
{
    m_uuid = Utils::GenerateUUID();
    m_light_type = LightTypeNone;
}

Light::Light(std::string name, const LightType &light_type)
    : m_name(name), m_light_type(light_type)
{
    m_uuid = Utils::GenerateUUID();
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

std::string Light::GetUUID() const
{
    return m_uuid;
}

PointLight::PointLight(std::string name) : Light(name, LightTypePoint)
{
}

PointLight::~PointLight()
{
}
