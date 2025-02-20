#ifndef __AXIS_H__
#define __AXIS_H__

class Shader;

class Axis
{
public:
    Axis();
    ~Axis();

    void init(int width, int height);

    void draw(long long elapsed);

private:
    Shader *shader_program;
    unsigned int VAO, VBO;
    // 创建 EBO 元素缓冲对象
    unsigned int EBO;
};

#endif