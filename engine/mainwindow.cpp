#include "globals.h"
#include "renderer.h"
#include "model/model.h"

#include "renderer_view.h"
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
    m_tree_view = new QTreeView(this);
    m_tree_view->setMaximumWidth(200);
    m_tree_view->setMinimumWidth(150);
    m_tree_view->setHeaderHidden(true);
    m_tree_view->expandAll();
    m_tree_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree_model = new QStandardItemModel(0, 1, this);
    m_tree_view->setModel(m_tree_model);

    m_property_view = new QTreeView(this);
    m_property_view->setMaximumWidth(200);
    m_property_view->setMinimumWidth(150);
    m_property_view->setHeaderHidden(true);

    // 创建一个分割器，将主窗口分为左右两部分
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_tree_view);
    splitter->addWidget(renderer_widget);
    splitter->addWidget(m_property_view);

    // 设置主窗口的中央部件为分割器
    this->setCentralWidget(splitter);

    QObject::connect(renderer_widget,
                     &RendererWidget::updateTreeListView,
                     this,
                     &ToyEngineMainWindow::onUpdateTreeListView);

    connect(this->m_tree_view,
            SIGNAL(clicked(QModelIndex)),
            this,
            SLOT(onTreeListMenuItemClicked()));
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

    for (int i = 0; i < m_tree_model->rowCount(); i++)
    {
        delete m_tree_model->item(i);
    }
    m_tree_model->clear();
    delete m_tree_model;
    delete m_tree_view;
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

// TreeView

void ToyEngineMainWindow::onTreeListMenuItemClicked()
{
    // 处理菜单点击事件
    qDebug() << "菜单项被点击";
    emit onUpdatePropertyView(m_tree_view->currentIndex().data().toString());
}

void ToyEngineMainWindow::onUpdatePropertyView(QString name)
{
    // 创一个treeview
    m_property_view->setModel(nullptr);
    m_property_view->setModel(new QStandardItemModel(0, 1, this));
    QStandardItemModel *model = new QStandardItemModel(0, 1, this);
    QStandardItem *rootItem = model->invisibleRootItem();
}

void ToyEngineMainWindow::onUpdateTreeListView()
{
    QStandardItem *rootItem = m_tree_model->invisibleRootItem();
    QStandardItem *modelsItem = new QStandardItem("模型");
    rootItem->appendRow(modelsItem);

    QStandardItem *lightsItem = new QStandardItem("灯光");
    rootItem->appendRow(lightsItem);

    for (auto model : gRenderer->GetModels())
    {
        auto item = new QStandardItem(QString::fromStdString(model->GetName()));
        modelsItem->appendRow(item);
    }
    m_tree_view->expandAll();
}