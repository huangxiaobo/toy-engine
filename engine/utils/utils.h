#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <glm/glm.hpp>
#include <tinyxml2/tinyxml2.h>

class Utils
{
public:
    static void DebugMatrix(const glm::mat4& mat);

    static std::string GetString(const glm::vec3 vec);   
    static std::string GetString(const glm::vec2 vec);   

    static glm::vec3 GetXYZ(tinyxml2::XMLElement *element);

    static glm::vec3 GetRGB(tinyxml2::XMLElement *element);
};

#endif