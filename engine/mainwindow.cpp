#include "renderer_view.h"
#include "treelist_view.h"

#include <QWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QObject>
#include <QStatusBar>
#include <QSplitter>
#include <QTreeView>
#include <QStandardItemModel>
#include <iostream>
#include "mainwindow.h"


ToyEngineMainWindow::ToyEngineMainWindow(QWidget *parent) : QMainWindow(parent)
{

    renderer_widget = new RendererWidget(this);

    // this->setCentralWidget(renderer_widget);
    // 设置窗口大小
    this->setGeometry(0, 0, 800, 600);
    // 创建定时器，定时刷新QOpenGLWidget
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]
            { 
                renderer_widget->update(); 
                renderer_widget->updateStatusBar(m_status_bar); });

    timer->start(33);
    m_status_bar = new QStatusBar(this);
    this->setStatusBar(m_status_bar);
    // m_status_bar->showMessage("hello world");
    QLabel *label = new QLabel("hello world", this);
    m_status_bar->addWidget(label);
    QLabel *fpsLabel = new QLabel("FPS: 0", this);
    m_status_bar->addWidget(fpsLabel);

    // 左侧树形组件
    m_tree_view = new TreeListView(this);
    
    // 创建一个分割器，将主窗口分为左右两部分
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_tree_view);
    splitter->addWidget(renderer_widget);

    // 设置主窗口的中央部件为分割器
    this->setCentralWidget(splitter);

    QObject::connect(renderer_widget, 
        &RendererWidget::updateTreeListView, 
        m_tree_view,
        &TreeListView::onUpdateTreeListView  
    );

}

ToyEngineMainWindow::~ToyEngineMainWindow()
{
    if (timer != nullptr)
    {
        timer->stop();
        delete timer;
        timer = nullptr;
    }

    if (renderer_widget != nullptr)
    {
        delete renderer_widget;
        renderer_widget = nullptr;
    }
    if (m_status_bar != nullptr)
    {
        delete m_status_bar;
        m_status_bar = nullptr;
    }
}

void ToyEngineMainWindow::closeEvent(QCloseEvent *event)
{
    if (timer != nullptr)
    {
        timer->stop();
        disconnect(timer, nullptr, nullptr, nullptr); // 断开所有信号连接
        delete timer;
        timer = nullptr;
    }
    std::cout << "close event" << std::endl;
    event->accept();
    qApp->quit();
}
