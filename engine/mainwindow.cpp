#include "globals.h"
#include "config.h"
#include "renderer.h"
#include "model/model.h"

#include "renderer_view.h"
#include <QWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QObject>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTreeView>
#include <QStandardItemModel>
#include <iostream>
#include "mainwindow.h"

ToyEngineMainWindow::ToyEngineMainWindow(QWidget *parent) : QMainWindow(parent)
{

    gConfig = Config::LoadFromXml("./resource/world.xml");

    // 菜单
    // 创建菜单栏
    QMenuBar *menuBar = new QMenuBar(this);
    menuBar->setNativeMenuBar(false);
    setMenuBar(menuBar);

    // 创建文件菜单
    QMenu *fileMenu = menuBar->addMenu("&文件");

    // 添加菜单项
    QAction *openAction = fileMenu->addAction("&打开");
    QAction *saveAction = fileMenu->addAction("&保存");
    QAction *exitAction = fileMenu->addAction("&退出");

    // 连接菜单项的点击事件到槽函数
    connect(openAction, &QAction::triggered, this, &ToyEngineMainWindow::onMenuOpen);
    connect(saveAction, &QAction::triggered, this, &ToyEngineMainWindow::onMenuSave);
    connect(exitAction, &QAction::triggered, this, &ToyEngineMainWindow::close);

    // 创建编辑菜单
    QMenu *editMenu = menuBar->addMenu("&引擎");

    // 添加菜单项
    QAction *redoAction = editMenu->addAction("&关于");

    // 连接菜单项的点击事件到槽函数
    connect(redoAction, &QAction::triggered, this, &ToyEngineMainWindow::onMenuAbout);

    renderer_widget = new RendererWidget(this);

    // this->setCentralWidget(renderer_widget);
    // 设置窗口大小
    this->setGeometry(
        0, 0,
        gConfig->Window.WindowWidth,
        gConfig->Window.WindowHeight);
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

void ToyEngineMainWindow::onMenuOpen()
{
    qDebug() << "打开菜单被点击";
}

void ToyEngineMainWindow::onMenuSave()
{
    qDebug() << "保存菜单被点击";
}

void ToyEngineMainWindow::onMenuAbout()
{
    qDebug() << "关于菜单被点击";
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