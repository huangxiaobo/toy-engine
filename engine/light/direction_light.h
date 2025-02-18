#ifndef __DIRECTION_LIGHT_H__

#include "light.h"

// 方向光源的属性
class DirLightAttr {
    public:
    QVector3D direction;
    QVector3D color;
    QVector3D ambient;
    QVector3D diffuse;
    QVector3D specular;
};
class DirectionLight : public Light {
public:
    DirectionLight();
    DirectionLight(const DirLightAttr &attr);

    ~DirectionLight();

    QVector3D getDirection() const;
    QVector3D getColor() const;
    QVector3D getAmbient() const;   
    QVector3D getDiffuse() const;
    QVector3D getSpecular() const;

private:
    DirLightAttr attr;
};

#endif#define __DIRECTION_LIGHT_H__