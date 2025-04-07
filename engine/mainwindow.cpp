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
#include <QColor>
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

    m_property_label_map.clear();
    for (auto m : m_property_browser->properties())
    {
        m_property_browser->removeProperty(m);
    }

    auto item_type = m_tree_view->currentIndex().data(Qt::UserRole + TreeItemTypeRoleOffset).toString().toStdString();
    auto item_uuid = m_tree_view->currentIndex().data(Qt::UserRole + TreeItemUUIDRoleOffset).toString().toStdString();

    if (item_type == TreeItemModel)
    {
        InitPropertyViewOfModel(item_uuid);
        return;
    }
    if (item_type == TreeItemLight)
    {
        InitPropertyViewOfLight(item_uuid);
        return;
    }
}

void ToyEngineMainWindow::AddProperty(PropertyType type, QString propertyName, bool bEditFlag, QString params)
{
}

void ToyEngineMainWindow::onPropertyChanged(QtProperty *property, const QVariant &value)
{
    // 获取当前选中的树形视图项
    QModelIndex currentIndex = m_tree_view->currentIndex();
    if (!currentIndex.isValid())
        return;

    PropertyLabel label = PROPERTY_LABEL_NONE;
    for (auto it = m_property_label_map.begin(); it != m_property_label_map.end(); ++it)
    {
        if (it->first == property)
        {
            label = it->second;
            break;
        }
    }
    if (label == PROPERTY_LABEL_NONE)
    {
        return;
    }

    // 获取选中项的类型和 UUID
    QString itemType = currentIndex.data(Qt::UserRole + TreeItemTypeRoleOffset).toString();
    QString itemUUID = currentIndex.data(Qt::UserRole + TreeItemUUIDRoleOffset).toString();

    if (itemType == QString::fromStdString(TreeItemModel))
    {
        // 找到对应的模型
        auto model = gRenderer->GetModelByUUID(itemUUID.toStdString());
        if (!model)
            return;

        // 根据属性名称更新模型状态
        auto position = model->GetPosition();
        auto scale = model->GetScale();
        switch (label)
        {
        case PROPERTY_LABEL_MODEL_POSITION_X:
            position.x = value.toDouble();
            model->SetPosition(position);
            break;
        case PROPERTY_LABEL_MODEL_POSITION_Y:
            position.y = value.toDouble();
            model->SetPosition(position);
            break;
        case PROPERTY_LABEL_MODEL_POSITION_Z:
            position.z = value.toDouble();
            model->SetPosition(position);
            break;
        case PROPERTY_LABEL_MODEL_SCALE_X:
            scale.x = value.toDouble();
            model->SetScale(scale);
            break;
        case PROPERTY_LABEL_MODEL_SCALE_Y:
            scale.y = value.toDouble();
            model->SetScale(scale);
            break;
        case PROPERTY_LABEL_MODEL_SCALE_Z:
            scale.z = value.toDouble();
            model->SetScale(scale);
            break;
        case PROPERTY_LABEL_MODEL_ROTATION:
            model->SetRotate(value.toFloat());
            break;
        default:
            break;
        }
    }
    if (itemType == QString::fromStdString(TreeItemLight))
    {
        // 找到对应的模型
        auto light = gRenderer->GetLightByUUID(itemUUID.toStdString());
        if (!light)
            return;

        auto lightType = light->GetLightType();
        if (lightType == LightType::LightTypePoint)
        {
            auto pointLight = dynamic_cast<PointLight *>(light);
            if (!pointLight)
                return;

            switch (label)
            {
            case PROPERTY_LABEL_LIGHT_COLOR_RGBA:
                pointLight->Color.r = value.value<QColor>().redF();
                pointLight->Color.g = value.value<QColor>().greenF();
                pointLight->Color.b = value.value<QColor>().blueF();
                break;
            case PROPERTY_LABEL_LIGHT_AMBIENT:
                pointLight->AmbientColor.r = value.value<QColor>().redF();
                pointLight->AmbientColor.g = value.value<QColor>().greenF();
                pointLight->AmbientColor.b = value.value<QColor>().blueF();
                break;
            case PROPERTY_LABEL_LIGHT_SPECULAR:
                pointLight->SpecularColor.r = value.value<QColor>().redF();
                pointLight->SpecularColor.g = value.value<QColor>().greenF();
                pointLight->SpecularColor.b = value.value<QColor>().blueF();
                break;
            case PROPERTY_LABEL_LIGHT_DIFFUSE:
                pointLight->DiffuseColor.r = value.value<QColor>().redF();
                pointLight->DiffuseColor.g = value.value<QColor>().greenF();
                pointLight->DiffuseColor.b = value.value<QColor>().blueF();
                break;
            default:
                break;
            }
        }
    }
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

void ToyEngineMainWindow::InitPropertyViewOfLight(string uuid)
{
    auto light = gRenderer->GetLightByUUID(uuid);

    if (light == nullptr)
    {
        return;
    }

    m_pVarMgrEdit = new QtVariantPropertyManager(m_property_view);     // 关联factory，属性可以修改
    m_pVarMgrOnlyRead = new QtVariantPropertyManager(m_property_view); // 这个管理器不关联factory，属性不可修改
    m_pVarFactory = new QtVariantEditorFactory(m_property_view);
    connect(m_pVarMgrEdit, &QtVariantPropertyManager::valueChanged, this, &ToyEngineMainWindow::onPropertyChanged); // 绑定信号槽，当值改变的时候会发送信号

    switch (light->GetLightType())
    {
    case LightType::LightTypePoint:
    {
        auto pointLight = dynamic_cast<PointLight *>(light);

        // 光源颜色
        auto color = pointLight->Color;
        QtVariantProperty *item_color = m_pVarMgrEdit->addProperty(QMetaType::QColor, QStringLiteral("光源颜色"));
        item_color->setValue(QColor(Qt::red, Qt::green, Qt::blue));
        item_color->setValue(QColor(color.r * 255, color.g * 255, color.b * 255));

        m_property_label_map.insert({item_color, PROPERTY_LABEL_LIGHT_COLOR_RGBA});
        m_property_browser->addProperty(item_color);

        // Ambient
        auto ambient = pointLight->AmbientColor;
        QtVariantProperty *item_ambient = m_pVarMgrEdit->addProperty(QMetaType::QColor, QStringLiteral("漫反射"));
        item_ambient->setValue(QColor(Qt::red, Qt::green, Qt::blue));
        item_ambient->setValue(QColor(ambient.r * 255, ambient.g * 255, ambient.b * 255));

        m_property_label_map.insert({item_ambient, PROPERTY_LABEL_LIGHT_AMBIENT});
        m_property_browser->addProperty(item_ambient);

        // Specular
        auto specularColor = pointLight->SpecularColor;
        QtVariantProperty *item_specular = m_pVarMgrEdit->addProperty(QMetaType::QColor, QStringLiteral("镜面反射"));
        item_specular->setValue(QColor(specularColor.r * 255, specularColor.g * 255, specularColor.b * 255));

        m_property_label_map.insert({item_specular, PROPERTY_LABEL_LIGHT_SPECULAR});
        m_property_browser->addProperty(item_specular);

        // Diffuse
        auto diffuseColor = pointLight->DiffuseColor;
        QtVariantProperty *item_diffuse = m_pVarMgrEdit->addProperty(QMetaType::QColor, QStringLiteral("散射"));
        item_diffuse->setValue(QColor(diffuseColor.r * 255, diffuseColor.g * 255, diffuseColor.b * 255));

        m_property_label_map.insert({item_diffuse, PROPERTY_LABEL_LIGHT_DIFFUSE});
        m_property_browser->addProperty(item_diffuse);
    }
    break;
    case LightType::LightTypeDirection:
        break;
    default:
        break;
    }

    m_property_browser->setFactoryForManager(m_pVarMgrEdit, m_pVarFactory); // 将一个工厂与manger关联起来，即可修改内容。
}

void ToyEngineMainWindow::InitPropertyViewOfModel(string modelUUID)
{
    auto model = gRenderer->GetModelByUUID(modelUUID);

    if (model == nullptr)
    {
        return;
    }

    m_pVarMgrEdit = new QtVariantPropertyManager(m_property_view);     // 关联factory，属性可以修改
    m_pVarMgrOnlyRead = new QtVariantPropertyManager(m_property_view); // 这个管理器不关联factory，属性不可修改
    m_pVarFactory = new QtVariantEditorFactory(m_property_view);
    connect(m_pVarMgrEdit, &QtVariantPropertyManager::valueChanged, this, &ToyEngineMainWindow::onPropertyChanged); // 绑定信号槽，当值改变的时候会发送信号

    // 位置
    QtProperty *groupPosition = m_pVarMgrEdit->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("位置"));

    QtVariantProperty *item_x = m_pVarMgrEdit->addProperty(QMetaType::Double, QStringLiteral("x"));
    item_x->setValue(model->GetPosition().x);
    groupPosition->addSubProperty(item_x);

    QtVariantProperty *item_y = m_pVarMgrEdit->addProperty(QMetaType::Double, QStringLiteral("y"));
    item_y->setValue(model->GetPosition().y);
    groupPosition->addSubProperty(item_y);

    QtVariantProperty *item_z = m_pVarMgrEdit->addProperty(QMetaType::Double, QStringLiteral("z"));
    item_z->setValue(model->GetPosition().z);
    groupPosition->addSubProperty(item_z);

    m_property_browser->addProperty(groupPosition);
    m_property_label_map.insert({item_x, PROPERTY_LABEL_MODEL_POSITION_X});
    m_property_label_map.insert({item_y, PROPERTY_LABEL_MODEL_POSITION_Y});
    m_property_label_map.insert({item_z, PROPERTY_LABEL_MODEL_POSITION_Z});

    // 缩放
    QtProperty *groupScale = m_pVarMgrOnlyRead->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("缩放"));

    QtVariantProperty *item_scale_x = m_pVarMgrOnlyRead->addProperty(QMetaType::Double, QStringLiteral("x"));
    item_scale_x->setValue(model->GetScale().x);
    groupScale->addSubProperty(item_scale_x);

    QtVariantProperty *item_scale_y = m_pVarMgrOnlyRead->addProperty(QMetaType::Double, QStringLiteral("y"));
    item_scale_y->setValue(model->GetScale().y);
    groupScale->addSubProperty(item_scale_y);

    QtVariantProperty *item_scale_z = m_pVarMgrOnlyRead->addProperty(QMetaType::Double, QStringLiteral("z"));
    item_scale_z->setValue(model->GetScale().z);
    groupScale->addSubProperty(item_scale_z);

    m_property_browser->addProperty(groupScale);
    m_property_label_map.insert({item_scale_x, PROPERTY_LABEL_MODEL_SCALE_X});
    m_property_label_map.insert({item_scale_y, PROPERTY_LABEL_MODEL_SCALE_Y});
    m_property_label_map.insert({item_scale_z, PROPERTY_LABEL_MODEL_SCALE_Z});

    // 旋转
    QtProperty *groupRotation = m_pVarMgrEdit->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("旋转"));

    QtVariantProperty *item_rotation = m_pVarMgrEdit->addProperty(QMetaType::Double, QStringLiteral("rotation"));
    item_rotation->setValue(model->GetRotation());
    groupRotation->addSubProperty(item_rotation);

    m_property_browser->addProperty(groupRotation);
    m_property_label_map.insert({item_rotation, PROPERTY_LABEL_MODEL_ROTATION});

    m_property_browser->setFactoryForManager(m_pVarMgrEdit, m_pVarFactory); // 将一个工厂与manger关联起来，即可修改内容。
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
        item->setData(QString::fromStdString(light->GetUUID()), Qt::UserRole + TreeItemUUIDRoleOffset);
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