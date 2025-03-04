#ifndef __MODEL_POINT_H__
#define __MODEL_POINT_H__

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "model.h"

class Light;

using namespace std;

class ModelPoint : public Model
{
public:
    ModelPoint(string name);
    ~ModelPoint();

    void Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::vec3 &camera, const std::vector<Light *> &lights);
};

#endif