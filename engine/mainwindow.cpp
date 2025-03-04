
#include <QWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include "mainwindow.h"
#include "renderer.h"

ToyEngineMainWindow::ToyEngineMainWindow(QWidget *parent) : QMainWindow(parent)
{

    renderer = new Renderer(this);

     this->setCentralWidget(renderer);
    //设置窗口大小
    this->setGeometry(0,0,800,600);
    //创建定时器，定时刷新QOpenGLWidget
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]{
        renderer->update();
    });

    timer->start(33);


}

ToyEngineMainWindow::~ToyEngineMainWindow()
{
    if (renderer != nullptr){
        delete renderer;
        renderer = nullptr;
    }
}

// void ToyEngineMainWindow::paintEvent(QPaintEvent *event)
// {
//     this->paintEvent(event);
// }