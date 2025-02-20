#ifndef __DIRECTION_LIGHT_H__

#include "light.h"
#include <glm/glm.hpp>

// 方向光源的属性
class DirLightAttr {
    public:
    glm::vec3 direction;
    glm::vec3 color;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
class DirectionLight : public Light {
public:
    DirectionLight();
    DirectionLight(const DirLightAttr &attr);

    ~DirectionLight();

    glm::vec3 getDirection() const;
    glm::vec3 getColor() const;
    glm::vec3 getAmbient() const;   
    glm::vec3 getDiffuse() const;
    glm::vec3 getSpecular() const;

private:
    DirLightAttr attr;
};

#endif //# define __DIRECTION_LIGHT_H__