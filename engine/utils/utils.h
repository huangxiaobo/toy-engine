#ifndef __UTILS_H__
#define __UTILS_H__

#include <glm/glm.hpp>
#include <tinyxml2/tinyxml2.h>

class Utils
{
public:
    static void DebugMatrix(const glm::mat4& mat);

    static glm::vec3 GetXYZ(tinyxml2::XMLElement *element);

    static glm::vec3 GetRGB(tinyxml2::XMLElement *element);
};

#endif