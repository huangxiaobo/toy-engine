#ifndef __AXIS_H__
#define __AXIS_H__

#include <QOpenGLShaderProgram>

class Axis
{
public:
    Axis();
    ~Axis();

    void init(int width, int height);

    void draw(long long elapsed);

private:
    QOpenGLShaderProgram *shader_program;
    GLuint VAO, VBO;
    // 创建 EBO 元素缓冲对象
    unsigned int EBO;
};

#endif