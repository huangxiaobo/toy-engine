#ifndef __POINT_LIGHT_H__

#include <QVector3D>

// 点光源
class PointLight : public Light {
public:
    PointLight(const QVector3D &position, const QVector3D &color, float intensity);

    float getIntensity(const QVector3D &point) ;

    QVector3D getPosition() const ;
};

#endif __POINT_LIGHT_H__