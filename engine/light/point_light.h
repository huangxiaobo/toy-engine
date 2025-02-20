#ifndef __POINT_LIGHT_H__

#include <glm/glm.hpp>
#include "light.h"

// 点光源
class PointLight : public Light {
public:
    PointLight(const glm::vec3 &position, const glm::vec3 &color, float intensity);

    float getIntensity(const glm::vec3 &point) ;

    glm::vec3 getPosition() const ;
};

#endif __POINT_LIGHT_H__