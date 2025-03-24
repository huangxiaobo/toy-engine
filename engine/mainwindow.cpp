#include "globals.h"
#include "config.h"
#include "renderer.h"
#include "model/model.h"
#include "light/light.h"

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
#include <qttreepropertybrowser.h>
#include <qtvariantproperty.h>
#include <qtpropertymanager.h>
#include <iostream>
#include "mainwindow.h"

const int TreeItemTypeRoleOffset = 0;
const int TreeItemUUIDRoleOffset = 1;
const std::string TreeItemModel = "Model";
const std::string TreeItemLight = "Light";

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
    m_property_view->setMaximumWidth(300);
    m_property_view->setMinimumWidth(150);
    m_property_view->setHeaderHidden(true);

    m_property_browser = new QtTreePropertyBrowser(m_property_view);
    m_property_browser->setMaximumWidth(200);
    m_property_browser->setMinimumWidth(150);
    // QtIntPropertyManager *intManager = new QtIntPropertyManager(m_property_view);
    // QtStringPropertyManager *stringManager = new QtStringPropertyManager(m_property_view);
    // QtVariantPropertyManager *variantManager = new QtVariantPropertyManager(m_property_view);

    // QtProperty *intProp = intManager->addProperty("Integer Property");
    // QtProperty *stringProp = stringManager->addProperty("String Property");
    // QtProperty *variantProp = variantManager->addProperty(1, "Variant Property");

    // m_property_browser->addProperty(intProp);
    // m_property_browser->addProperty(stringProp);
    // m_property_browser->addProperty(variantProp);

    // 创建一个分割器，将主窗口分为左右两部分
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_tree_view);
    splitter->addWidget(renderer_widget);
    splitter->addWidget(m_property_browser);

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
    qDebug() << "菜单项被点击" << m_tree_view->currentIndex().data().toString();
    qDebug() << m_tree_view->currentIndex().data(Qt::UserRole);
    emit onUpdatePropertyView(m_tree_view->currentIndex().data().toString());
}

void ToyEngineMainWindow::onUpdatePropertyView(QString name)
{
    // 创一个treeview
    // m_property_view->setModel(nullptr);
    // m_property_view->setModel(new QStandardItemModel(0, 1, this));
    // QStandardItemModel *model = new QStandardItemModel(0, 1, this);
    // QStandardItem *rootItem = model->invisibleRootItem();

    for (auto m : m_property_browser->properties())
    {
        m_property_browser->removeProperty(m);
    }

    auto ItemType = m_tree_view->currentIndex().data(Qt::UserRole + TreeItemTypeRoleOffset).toString().toStdString();
    if (ItemType != TreeItemModel)
    {
        return;
    }
    auto model_uuid = m_tree_view->currentIndex().data(Qt::UserRole + TreeItemUUIDRoleOffset).toString().toStdString();
    auto model = gRenderer->GetModelByUUID(model_uuid);

    if (model == nullptr)
    {
        return;
    }

    QtVariantPropertyManager *m_pVarManager = new QtVariantPropertyManager(m_property_browser);

    // group 1 ----------------------------------------------------------------------------------------------------------
    QtProperty *groupPosition = m_pVarManager->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("位置"));

    QtVariantProperty *item_x = m_pVarManager->addProperty(QVariant::Double, QStringLiteral("x: "));
    item_x->setValue(model->GetPosition().x);
    groupPosition->addSubProperty(item_x);

    QtVariantProperty *item_y = m_pVarManager->addProperty(QVariant::Double, QStringLiteral("y: "));
    item_y->setValue(model->GetPosition().y);
    groupPosition->addSubProperty(item_y);

    QtVariantProperty *item_z = m_pVarManager->addProperty(QVariant::Double, QStringLiteral("z: "));
    item_z->setValue(model->GetPosition().z);
    groupPosition->addSubProperty(item_z);

    m_property_browser->addProperty(groupPosition);

    // 缩放
    // group 1 ----------------------------------------------------------------------------------------------------------
    QtProperty *groupScale = m_pVarManager->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("缩放"));

    QtVariantProperty *item_scale_x = m_pVarManager->addProperty(QVariant::Double, QStringLiteral("x: "));
    item_scale_x->setValue(model->GetScale().x);
    groupScale->addSubProperty(item_scale_x);

    QtVariantProperty *item_scale_y = m_pVarManager->addProperty(QVariant::Double, QStringLiteral("y: "));
    item_scale_y->setValue(model->GetScale().y);
    groupScale->addSubProperty(item_scale_y);

    QtVariantProperty *item_scale_z = m_pVarManager->addProperty(QVariant::Double, QStringLiteral("z: "));
    item_scale_z->setValue(model->GetScale().z);
    groupScale->addSubProperty(item_scale_z);

    m_property_browser->addProperty(groupScale);
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
        item->setData(QString::fromStdString(TreeItemModel), Qt::UserRole + TreeItemTypeRoleOffset);
        item->setData(QString::fromStdString(model->GetUUID()), Qt::UserRole + TreeItemUUIDRoleOffset);
        modelsItem->appendRow(item);
    }

    for (auto light : gRenderer->GetLights())
    {
        std::cout << light->GetName() << std::endl;

        auto item = new QStandardItem(QString::fromStdString(light->GetName()));
        item->setData(QString::fromStdString(TreeItemLight), Qt::UserRole + TreeItemTypeRoleOffset);
        item->setData(QString::fromStdString(""), Qt::UserRole + TreeItemUUIDRoleOffset);
        lightsItem->appendRow(item);
    }
    m_tree_view->expandAll();

    // 设置默认选中
    if (modelsItem->rowCount() > 0)
    {
        QModelIndex firstModelIndex = modelsItem->child(0)->index();
        m_tree_view->setCurrentIndex(firstModelIndex);

        // 触发点击事件
        QItemSelectionModel *selectionModel = m_tree_view->selectionModel();
        selectionModel->select(firstModelIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        emit m_tree_view->clicked(firstModelIndex);
    }
}