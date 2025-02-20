#ifndef __GROUND_H__
#define __GROUND_H__

#include "mesh.h"
#include <glm/glm.hpp>

class Ground : public Mesh
{
public:
    Ground();
    void Init();
    void Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::vec3 &camera);
};

#endif