#include "renderer_view.h"
#include "renderer.h"
#include <iostream>

#include <QLabel>
#include <QTimer>

RendererWidget::RendererWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    this->m_time.start();
    // 使 widget 可以接收键盘焦点
    setFocusPolicy(Qt::StrongFocus);

    m_timer = new QTimer(this);
    m_timer->setInterval(25);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateGL()),Qt::QueuedConnection);

    m_fps_label = new QLabel(this);
    m_fps_label->setStyleSheet("QLabel { color : white; }");
    m_fps_label->setGeometry(10, 10, 100, 20);
    m_fps_label->setText("FPS: 0");
    m_fps_label->setFont(QFont("Arial", 20));

    m_fps_timer = new QTimer(this);
    m_frame_count = 0;
    connect(m_fps_timer, &QTimer::timeout, this, [this]
            {
      double elapsed = m_time.elapsed();
      if (elapsed > 0)
      {
         auto frame_count_delta = m_render->GetFrameCount() - m_frame_count;
         m_frame_count += frame_count_delta;
         
         auto fps = 1.0f * frame_count_delta / (elapsed / 1000.0f);
         m_fps_label->setText(QString("FPS: %1").arg(fps));
      } });
    m_fps_timer->start(1000);
}

RendererWidget::~RendererWidget()
{
    makeCurrent();
    if (m_render != nullptr)
    {
        delete m_render;
        m_render = nullptr;
    }
    if (m_time.isValid())
    {
        m_time.invalidate();
    }
    if (m_timer != nullptr)
    {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
    if (m_fps_timer != nullptr)
    {
        m_fps_timer->stop();
        delete m_fps_timer;
        m_fps_timer = nullptr;
    }
    doneCurrent();
}

void RendererWidget::initializeGL()
{
    // 使用glad原生OpenGL, 无需初始化QT的OpenGL函数
    // initializeOpenGLFunctions();

    this->m_render = new Renderer();
    m_render->init(width(), height());
}

void RendererWidget::paintGL()
{
    m_render->draw(m_time.elapsed());
}

void RendererWidget::resizeGL(int w, int h)
{
    m_render->resize(w, h);
}

void RendererWidget::keyPressEvent(QKeyEvent *e)
{
    std::cout << "keyPressEvent" << std::endl;
    switch (e->key())
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
