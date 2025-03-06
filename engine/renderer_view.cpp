#include "renderer_view.h"
#include "renderer.h"
#include <iostream>

#include <QTime>

RendererWidget::RendererWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    this->m_time.start();
    // 使 widget 可以接收键盘焦点
    setFocusPolicy(Qt::StrongFocus);
}

RendererWidget::~RendererWidget()
{
    makeCurrent();
   if (render != nullptr)
    {
        delete render;
        render = nullptr;
    }
    if (m_time.isValid())
    {
        m_time.invalidate();
    }
    doneCurrent();
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

void RendererWidget::keyPressEvent(QKeyEvent *e)
{
    std::cout<<"keyPressEvent"<<std::endl;
    switch ( e->key() )
  {
  case Qt::Key_L:
    break;

  case Qt::Key_F:
    break;

  case Qt::Key_W:
    break;

  case Qt::Key_S:
    break;

  case Qt::Key_Up:
    break;

  case Qt::Key_Down:
    break;

  case Qt::Key_Right:
    break;

  case Qt::Key_Left:
    break;

  case Qt::Key_A:
    break;

  case Qt::Key_D:
    break;


  case Qt::Key_Escape:
    close();
    qApp->quit();
    break;

  }
}
