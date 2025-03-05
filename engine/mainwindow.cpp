
#include <QWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <iostream>
#include "mainwindow.h"
#include "renderer_view.h"

ToyEngineMainWindow::ToyEngineMainWindow(QWidget *parent) : QMainWindow(parent)
{

    renderer_widget = new RendererWidget(this);

     this->setCentralWidget(renderer_widget);
    //设置窗口大小
    this->setGeometry(0,0,800,600);
    //创建定时器，定时刷新QOpenGLWidget
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]{
        renderer_widget->update();
    });

    timer->start(33);

}

ToyEngineMainWindow::~ToyEngineMainWindow()
{
    if (timer != nullptr){
        timer->stop();
        delete timer;
        timer = nullptr;
    }

    if (renderer_widget != nullptr){
        delete renderer_widget;
        renderer_widget = nullptr;
    }
}

void ToyEngineMainWindow::closeEvent(QCloseEvent *event)
{
    if (timer != nullptr){
        timer->stop();
        disconnect(timer, nullptr, nullptr, nullptr); // 断开所有信号连接
        delete timer;
        timer = nullptr;
    }
    std::cout << "close event" << std::endl;
    event->accept();
    qApp->quit();
}
