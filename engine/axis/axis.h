#ifndef __AXIS_H__
#define __AXIS_H__

#include <glm/glm.hpp>

class Shader;

class Axis
{
public:
    Axis();
    ~Axis();

    void init(int width, int height);

    void draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model);

private:
    Shader *shader_program;
    unsigned int VAO, VBO;
    // 创建 EBO 元素缓冲对象
    unsigned int EBO;
};

#endif