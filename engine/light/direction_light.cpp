#include "direction_light.h"

#include <glm/glm.hpp>

DirectionLight::DirectionLight()
{
}

DirectionLight::DirectionLight(const DirLightAttr &attr):Light(LightTypeDirection)
{
    this->attr = attr;
}

DirectionLight::~DirectionLight()
{
}

glm::vec3 DirectionLight::getDirection() const
{
    return attr.direction;
}

glm::vec3 DirectionLight::getColor() const
{
    return attr.color;
}

glm::vec3 DirectionLight::getAmbient() const
{
    return attr.ambient;
}

glm::vec3 DirectionLight::getDiffuse() const
{
    return attr.diffuse;
}

glm::vec3 DirectionLight::getSpecular() const
{
    return attr.specular;
}
