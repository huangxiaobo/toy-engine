#include "model_ground.h"
#include "../light/light.h"
#include "../mesh/mesh.h"
#include "model.h"

#include <iostream>

ModelGround::ModelGround() : Model("ground")
{
}

ModelGround::~ModelGround()
{
}

void ModelGround::Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::vec3 &camera, const std::vector<Light *> &lights)
{
    glLineWidth(2.3f);
    Model::Draw(elapsed, projection, view, model, camera, lights);
}
