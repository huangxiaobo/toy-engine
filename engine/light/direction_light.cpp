#include "direction_light.h"

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

QVector3D DirectionLight::getDirection() const
{
    return attr.direction;
}

QVector3D DirectionLight::getColor() const
{
    return attr.color;
}

QVector3D DirectionLight::getAmbient() const
{
    return attr.ambient;
}

QVector3D DirectionLight::getDiffuse() const
{
    return attr.diffuse;
}

QVector3D DirectionLight::getSpecular() const
{
    return attr.specular;
}
