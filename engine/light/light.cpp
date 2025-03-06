#include "light.h"

Light::Light()
{
    m_light_type = LightTypeNone;
}

PointLight::PointLight(): Light(LightTypePoint) 
{
}

PointLight::~PointLight()
{
}

