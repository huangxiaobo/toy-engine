#include "globals.h"
#include "renderer.h"
#include "camera/camera.h"
#include "model/model.h"
#include <iostream>

#include <QLabel>
#include <QTimer>
#include <QStandardItemModel>
#include <QStandardItem>

#include "renderer_view.h"

RendererWidget::RendererWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    this->m_time.start();
    // 使 widget 可以接收键盘焦点
    setFocusPolicy(Qt::StrongFocus);

    m_timer = new QTimer(this);
    m_timer->setInterval(25);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateGL()), Qt::QueuedConnection);

    m_fps_label = new QLabel(this);
    m_fps_label->setStyleSheet("QLabel { color : white; }");
    m_fps_label->setGeometry(10, 10, 100, 20);
    m_fps_label->setText("FPS: 0");
    m_fps_label->setFont(QFont("Arial", 20));

    m_fps_timer = new QTimer(this);
    m_frame_count = 0;
    connect(m_fps_timer, &QTimer::timeout, this, [this]
            {
         auto fps = gRenderer->GetFPS();
         m_fps_label->setText(QString("FPS: %1").arg(fps)); });
    m_fps_timer->start(1000);
}

RendererWidget::~RendererWidget()
{
    makeCurrent();
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

void RendererWidget::updateStatusBar(QStatusBar *status_bar)
{
    QList<QLabel *> labels = status_bar->findChildren<QLabel *>();
    if (labels.size() >= 2)
    {
        labels[0]->setText(QString("帧率: %1").arg(gRenderer->GetFPS()));
        labels[1]->setText(QString("模型数量: %1").arg(gRenderer->GetModelCount()));
    }
}

void RendererWidget::updateModelList(QTreeView *tree_view)
{
    
}

void RendererWidget::initializeGL()
{
    // 使用glad原生OpenGL, 无需初始化QT的OpenGL函数
    // initializeOpenGLFunctions();

    gRenderer->init(width(), height());
    emit updateTreeListView();
}

void RendererWidget::paintGL()
{
    auto elapsed = m_time.elapsed();
    gRenderer->update(elapsed);
    gRenderer->draw(elapsed);
}

void RendererWidget::resizeGL(int w, int h)
{
    gRenderer->resize(w, h);
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
        gRenderer->GetCamera()->ProcessKeyboard(CameraMoveType::FORWARD, 0.1f);
        break;

    case Qt::Key_Down:
        gRenderer->GetCamera()->ProcessKeyboard(CameraMoveType::BACKWARD, 0.1f);
        break;

    case Qt::Key_Right:
        gRenderer->GetCamera()->ProcessKeyboard(CameraMoveType::RIGHT, 0.1f);
        break;

    case Qt::Key_Left:
    gRenderer->GetCamera()->ProcessKeyboard(CameraMoveType::LEFT, 0.1f);
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
