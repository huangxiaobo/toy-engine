#include "model_point.h"

#include <iostream>
#include "model.h"

ModelPoint::ModelPoint(string name): Model(name)
{
}
ModelPoint::~ModelPoint()
{
}
void ModelPoint::Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::vec3 &camera, const std::vector<Light *> &lights)
{
    glPointSize(50.0f);
    Model::Draw(elapsed, projection, view, model, camera, lights);
}
