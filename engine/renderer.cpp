

#include "renderer.h"

Renderer::Renderer(QWidget *parent) : QOpenGLWidget(parent)
{

}

Renderer::~Renderer()
{

}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void Renderer::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}