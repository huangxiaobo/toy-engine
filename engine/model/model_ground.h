#ifndef __MODEL_GROUND_H__
#define __MODEL_GROUND_H__

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "model.h"

using namespace std;

class Light;

class ModelGround : public Model
{
public:
    // 构造函数
    ModelGround();
    // 析构函数
    ~ModelGround();

    // 重写父类的Draw方法， 画地面
    void Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::vec3 &camera, const std::vector<Light *> &lights);
};

#endif