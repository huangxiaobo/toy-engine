#include "renderer_view.h"
#include "renderer.h"
#include <iostream>

#include <QTime>

RendererWidget::RendererWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    this->m_time.start();
}

RendererWidget::~RendererWidget()
{
   if (render != nullptr)
    {
        delete render;
    }
}

void RendererWidget::initializeGL()
{
    // 使用glad原生OpenGL, 无需初始化QT的OpenGL函数
    // initializeOpenGLFunctions();

    this->render = new Renderer();
    render->init(width(), height());
}

void RendererWidget::paintGL()
{
    render->draw(m_time.elapsed());
}

void RendererWidget::resizeGL(int w, int h)
{
    render->resize(w, h);
}
