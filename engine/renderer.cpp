
#include <iostream>
#include "renderer.h"
#include <QTime>

Renderer::Renderer(QWidget *parent) : QOpenGLWidget(parent)
{
    this->m_time.start();
}

Renderer::~Renderer()
{
    if (m_axis != nullptr)
    {
        delete m_axis;
    }
}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();

    const char *version = (const char *)glGetString(GL_VERSION);
    std::cout << "OpenGL version: " << version << std::endl;

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_PROGRAM_POINT_SIZE);

    m_axis = new Axis();
    m_axis->init(width(), height());
}

void Renderer::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    this->m_axis->draw(this->m_time.elapsed());
}

void Renderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
