#include "globals.h"
#include "config.h"
#include "renderer.h"
#include "model/model.h"
#include "light/light.h"

#include "renderer_view.h"
#include <QWidget>
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QObject>
#include <QMessageBox>
#include <QToolBar>
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
#include <QUuid>

const int TreeItemTypeRoleOffset = 0;
const int TreeItemUUIDRoleOffset = 1;
const std::string TreeItemModel = "Model";
const std::string TreeItemLight = "Light";

ToyEngineMainWindow::ToyEngineMainWindow(QWidget *parent) : QMainWindow(parent) {
    gConfig = Config::LoadFromYaml("./resource/world.yaml");

    // 设置窗口大小
    this->setGeometry(
        0, 0,
        gConfig->Window.WindowWidth,
        gConfig->Window.WindowHeight
    );

    InitMenuBar();
    InitStatusBar();

    // 创建定时器，定时刷新QOpenGLWidget
    m_renderer_widget = new RendererWidget(this);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
        m_renderer_widget->update();
        m_renderer_widget->updateStatusBar(m_status_bar);
    });


    timer->start(33);


    // 左侧树形组件
    m_tree_view = new QTreeView(this);
    m_tree_view->setMaximumWidth(200);
    m_tree_view->setMinimumWidth(150);
    m_tree_view->setHeaderHidden(true);
    m_tree_view->expandAll();
    m_tree_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree_model = new QStandardItemModel(0, 1, this);
    m_tree_view->setModel(m_tree_model);

    m_property_browser = new QtTreePropertyBrowser(this);
    m_property_browser->setMaximumWidth(200);
    m_property_browser->setMinimumWidth(150);
    m_var_prop_mgr_edit = new QtVariantPropertyManager(m_property_browser); // 关联factory，属性可以修改
    m_var_prop_mgr_onlyread = new QtVariantPropertyManager(m_property_browser); // 这个管理器不关联factory，属性不可修改
    m_var_edit_factory = new QtVariantEditorFactory(m_property_browser);
    m_property_browser->setFactoryForManager(m_var_prop_mgr_edit, m_var_edit_factory); // 将一个工厂与manger关联起来，即可修改内容。


    // 创建一个分割器，将主窗口分为左右两部分
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_tree_view);
    splitter->addWidget(m_renderer_widget);
    splitter->addWidget(m_property_browser);

    // 设置主窗口的中央部件为分割器
    this->setCentralWidget(splitter);

    connect(this->m_renderer_widget,
            &RendererWidget::updateTreeListView,
            this,
            &ToyEngineMainWindow::onUpdateTreeListView);

    connect(this->m_tree_view,
            SIGNAL(clicked(QModelIndex)),
            this,
            SLOT(onTreeListMenuItemClicked()));
}

ToyEngineMainWindow::~ToyEngineMainWindow() {
    if (timer != nullptr) {
        timer->stop();
        delete timer;
        timer = nullptr;
    }

    if (m_renderer_widget != nullptr) {
        delete m_renderer_widget;
        m_renderer_widget = nullptr;
    }
    if (m_status_bar != nullptr) {
        delete m_status_bar;
        m_status_bar = nullptr;
    }
    if (m_menu_bar != nullptr) {
        delete m_menu_bar;
        m_menu_bar = nullptr;
    }

    for (int i = 0; i < m_tree_model->rowCount(); i++) {
        delete m_tree_model->item(i);
    }
    m_tree_model->clear();
    delete m_tree_model;
    delete m_tree_view;
}

void ToyEngineMainWindow::closeEvent(QCloseEvent *event) {
    if (timer != nullptr) {
        timer->stop();
        QObject::disconnect(timer, nullptr, nullptr, nullptr); // 断开所有信号连接
        delete timer;
        timer = nullptr;
    }
    std::cout << "close event" << std::endl;
    event->accept();
    qApp->quit();
}


void ToyEngineMainWindow::onTreeListMenuItemClicked() {
    // 处理菜单点击事件
    qDebug() << "菜单项被点击" << m_tree_view->currentIndex().data().toString();
    emit onUpdatePropertyView(m_tree_view->currentIndex().data().toString());
    if (m_tree_view->currentIndex().data().toString() == "场景") {
        gRenderer->LoadWorldFromFile("./resource/world.yaml");
    }
}

void ToyEngineMainWindow::onUpdatePropertyView(QString name) {
    m_property_label_map.clear();

    for (auto m: m_property_browser->properties()) {
        m_property_browser->removeProperty(m);
    }
    m_var_prop_mgr_edit->clear();
    m_var_prop_mgr_onlyread->clear();


    auto item_type = m_tree_view->currentIndex().data(Qt::UserRole + TreeItemTypeRoleOffset).toString().toStdString();
    auto item_uuid = m_tree_view->currentIndex().data(Qt::UserRole + TreeItemUUIDRoleOffset).toString().toStdString();

    if (item_type == TreeItemModel) {
        InitPropertyViewOfModel(item_uuid);
        return;
    }
    if (item_type == TreeItemLight) {
        InitPropertyViewOfLight(item_uuid);
        return;
    }
}

void ToyEngineMainWindow::onMenuAbout() {
    qDebug() << "关于菜单被点击";
    QString aboutText = "<h2>Toy Engine</h2>"
            "<p>版本 1.0.0<br/>"
            "作者：<b>开发者团队</b><br/>"
            "描述：这是一个轻量级玩具引擎，用于学习和演示3D渲染基础。</p>"
            "<p>© 2025 Toy Engine 开发组.</p>";

    QMessageBox::about(this, tr("关于 Toy Engine"), aboutText);
}

void ToyEngineMainWindow::InitMenuBar() {
    // 菜单
    m_menu_bar = new QMenuBar(this);
    m_menu_bar->setNativeMenuBar(false);
    setMenuBar(m_menu_bar);

    // 创建文件菜单
    QMenu *fileMenu = m_menu_bar->addMenu("&引擎");
    QAction *aboutAction = fileMenu->addAction("&关于");
    QAction *exitAction = fileMenu->addAction("&退出");
    QAction *projAction = fileMenu->addAction("&切换投影");
    m_menu_bar->setVisible(false);

    aboutAction->setIcon(QIcon("./resource/info.png"));
    exitAction->setIcon(QIcon("./resource/exit.png"));
    projAction->setIcon(QIcon("./resource/video-camera.png"));

    // 连接菜单项的点击事件到槽函数
    connect(exitAction, &QAction::triggered, this, [=]() {
        close();
    });
    connect(aboutAction, &QAction::triggered, this, [=]() {
        onMenuAbout();
    });
    connect(projAction, &QAction::triggered, this, [=]() {
        switch (m_renderer_widget->GetProjectionType()) {
            case ProjectionType::Orthographic:
                m_renderer_widget->SetProjectionType(ProjectionType::Perspective);
                break;
            case ProjectionType::Perspective:
                m_renderer_widget->SetProjectionType(ProjectionType::Orthographic);
                break;
        }
    });

    // 创建工具栏
    auto *toolBar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, toolBar);

    toolBar->addAction(aboutAction);
    toolBar->addAction(projAction);
    toolBar->addAction(exitAction);

    // 设置禁止移动属性,工具栏默认贴在上方
    toolBar->setFloatable(false);
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void ToyEngineMainWindow::InitStatusBar() {
    m_status_bar = new QStatusBar(this);
    this->setStatusBar(m_status_bar);

    auto *fpsLabel = new QLabel("FPS: 0", this);
    m_status_bar->addWidget(fpsLabel);
}

void ToyEngineMainWindow::InitPropertyViewOfLight(const string &uuid) {
    auto light = gRenderer->GetLightByUUID(uuid);

    if (light == nullptr) {
        return;
    }

    switch (light->GetLightType()) {
        case LightType::LightTypePoint: {
            auto pointLight = dynamic_cast<PointLight *>(light);

            // 光源颜色
            auto color = pointLight->Color;
            QtVariantProperty *item_color = m_var_prop_mgr_edit->addProperty(QMetaType::QColor, QStringLiteral("光源颜色"));
            item_color->setValue(QColor(color.r * 255, color.g * 255, color.b * 255));
            m_property_label_map.insert({item_color, PROPERTY_LABEL_LIGHT_COLOR_RGBA});
            m_property_browser->addProperty(item_color);

            // Ambient
            auto ambient = pointLight->AmbientColor;
            QtVariantProperty *item_ambient = m_var_prop_mgr_edit->addProperty(QMetaType::QColor, QStringLiteral("漫反射"));
            item_ambient->setValue(QColor(ambient.r * 255, ambient.g * 255, ambient.b * 255));
            m_property_label_map.insert({item_ambient, PROPERTY_LABEL_LIGHT_AMBIENT});
            m_property_browser->addProperty(item_ambient);

            // Specular
            auto specularColor = pointLight->SpecularColor;
            QtVariantProperty *item_specular = m_var_prop_mgr_edit->addProperty(QMetaType::QColor, QStringLiteral("镜面反射"));
            m_property_label_map.insert({item_specular, PROPERTY_LABEL_LIGHT_SPECULAR});
            item_specular->setValue(QColor(specularColor.r * 255, specularColor.g * 255, specularColor.b * 255));

            m_property_browser->addProperty(item_specular);

            // Diffuse
            auto diffuseColor = pointLight->DiffuseColor;
            QtVariantProperty *item_diffuse = m_var_prop_mgr_edit->addProperty(QMetaType::QColor, QStringLiteral("散射"));
            m_property_label_map.insert({item_diffuse, PROPERTY_LABEL_LIGHT_DIFFUSE});
            item_diffuse->setValue(QColor(diffuseColor.r * 255, diffuseColor.g * 255, diffuseColor.b * 255));

            m_property_browser->addProperty(item_diffuse);
        }
        break;
        case LightType::LightTypeDirection:
            break;
        default:
            break;
    }


    // 绑定信号槽，当值改变的时候会发送信号
    connect(m_var_prop_mgr_edit, &QtVariantPropertyManager::valueChanged, this,
            [=](QtProperty *property, const QVariant &value) {
                onLightPropertyChanged(uuid, property, value);
            }
    );
}

void ToyEngineMainWindow::onLightPropertyChanged(const string &uuid,
                                                 QtProperty *property,
                                                 const QVariant &value) {
    // 找到对应的模型
    auto light = gRenderer->GetLightByUUID(uuid);
    if (!light)
        return;
    if (!m_property_label_map.contains(property)) {
        return;
    }

    auto label = m_property_label_map[property];

    auto lightType = light->GetLightType();
    if (lightType == LightType::LightTypePoint) {
        auto pointLight = dynamic_cast<PointLight *>(light);
        if (!pointLight)
            return;

        switch (label) {
            case PROPERTY_LABEL_LIGHT_COLOR_RGBA:
                // 处理光源颜色
                pointLight->Color.r = value.value<QColor>().redF();
                pointLight->Color.g = value.value<QColor>().greenF();
                pointLight->Color.b = value.value<QColor>().blueF();
                break;
            case PROPERTY_LABEL_LIGHT_AMBIENT:

                // 处理漫反射
                pointLight->AmbientColor.r = value.value<QColor>().redF();
                pointLight->AmbientColor.g = value.value<QColor>().greenF();
                pointLight->AmbientColor.b = value.value<QColor>().blueF();
                break;
            case PROPERTY_LABEL_LIGHT_SPECULAR:
                // 处理镜面反射
                pointLight->SpecularColor.r = value.value<QColor>().redF();
                pointLight->SpecularColor.g = value.value<QColor>().greenF();
                pointLight->SpecularColor.b = value.value<QColor>().blueF();
                break;
            case PROPERTY_LABEL_LIGHT_DIFFUSE:
                // 处理散射
                pointLight->DiffuseColor.r = value.value<QColor>().redF();
                pointLight->DiffuseColor.g = value.value<QColor>().greenF();
                pointLight->DiffuseColor.b = value.value<QColor>().blueF();
                break;
            default:
                break;
        }
    }
}


void ToyEngineMainWindow::InitPropertyViewOfModel(const string& modelUUID) {
    auto model = gRenderer->GetModelByUUID(modelUUID);

    if (model == nullptr) {
        return;
    }

    // 位置
    QtProperty *groupPosition = m_var_prop_mgr_edit->addProperty(QtVariantPropertyManager::groupTypeId(),
                                                           QStringLiteral("位置"));

    QtVariantProperty *item_x = m_var_prop_mgr_edit->addProperty(QMetaType::Double, QStringLiteral("x"));
    item_x->setValue(model->GetPosition().x);
    groupPosition->addSubProperty(item_x);

    QtVariantProperty *item_y = m_var_prop_mgr_edit->addProperty(QMetaType::Double, QStringLiteral("y"));
    item_y->setValue(model->GetPosition().y);
    groupPosition->addSubProperty(item_y);

    QtVariantProperty *item_z = m_var_prop_mgr_edit->addProperty(QMetaType::Double, QStringLiteral("z"));
    item_z->setValue(model->GetPosition().z);
    groupPosition->addSubProperty(item_z);

    m_property_browser->addProperty(groupPosition);
    m_property_label_map.insert({item_x, PROPERTY_LABEL_MODEL_POSITION_X});
    m_property_label_map.insert({item_y, PROPERTY_LABEL_MODEL_POSITION_Y});
    m_property_label_map.insert({item_z, PROPERTY_LABEL_MODEL_POSITION_Z});

    // 缩放
    QtProperty *groupScale = m_var_prop_mgr_onlyread->addProperty(QtVariantPropertyManager::groupTypeId(),
                                                            QStringLiteral("缩放"));

    QtVariantProperty *item_scale_x = m_var_prop_mgr_onlyread->addProperty(QMetaType::Double, QStringLiteral("x"));
    item_scale_x->setValue(model->GetScale().x);
    groupScale->addSubProperty(item_scale_x);

    QtVariantProperty *item_scale_y = m_var_prop_mgr_onlyread->addProperty(QMetaType::Double, QStringLiteral("y"));
    item_scale_y->setValue(model->GetScale().y);
    groupScale->addSubProperty(item_scale_y);

    QtVariantProperty *item_scale_z = m_var_prop_mgr_onlyread->addProperty(QMetaType::Double, QStringLiteral("z"));
    item_scale_z->setValue(model->GetScale().z);
    groupScale->addSubProperty(item_scale_z);

    m_property_browser->addProperty(groupScale);
    m_property_label_map.insert({item_scale_x, PROPERTY_LABEL_MODEL_SCALE_X});
    m_property_label_map.insert({item_scale_y, PROPERTY_LABEL_MODEL_SCALE_Y});
    m_property_label_map.insert({item_scale_z, PROPERTY_LABEL_MODEL_SCALE_Z});

    // 旋转
    QtProperty *groupRotation = m_var_prop_mgr_edit->addProperty(QtVariantPropertyManager::groupTypeId(),
                                                           QStringLiteral("旋转"));

    QtVariantProperty *item_rotation = m_var_prop_mgr_edit->addProperty(QMetaType::Double, QStringLiteral("rotation"));
    item_rotation->setValue(model->GetRotation());
    groupRotation->addSubProperty(item_rotation);

    m_property_browser->addProperty(groupRotation);
    m_property_label_map.insert({item_rotation, PROPERTY_LABEL_MODEL_ROTATION});

    // 绑定信号槽，当值改变的时候会发送信号
    disconnect(m_var_prop_mgr_edit, nullptr, nullptr, nullptr);
    connect(m_var_prop_mgr_edit, &QtVariantPropertyManager::valueChanged, this,
            [=](QtProperty *property, const QVariant &value) {
                onModelPropertyChanged(modelUUID, property, value);
            }
    );
}


void ToyEngineMainWindow::onModelPropertyChanged(const string& modelUUID, QtProperty *property, const QVariant &value) {
    // 找到对应的模型
    auto model = gRenderer->GetModelByUUID(modelUUID);
    if (!model)
        return;
    if (!m_property_label_map.contains(property)) {
        return;
    }
    auto label = m_property_label_map[property];

    // 根据属性名称更新模型状态
    auto position = model->GetPosition();
    auto scale = model->GetScale();
    switch (label) {
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


void ToyEngineMainWindow::onUpdateTreeListView() {
    QStandardItem *rootItem = m_tree_model->invisibleRootItem();

    // QStandardItem *item = new QStandardItem("场景");

    // // 设置项的选中状态
    // item->setCheckable(true);
    // item->setData(Qt::Checked, Qt::CheckStateRole);

    // // 获取并打印选中状态
    // QVariant checkedState = item->data(Qt::CheckStateRole);
    // qDebug() << "选中状态：" << checkedState.toInt();
    // rootItem->appendRow(item);
    m_tree_view->setIndentation(8);

    QStandardItem *modelsItem = new QStandardItem("模型");
    rootItem->appendRow(modelsItem);

    QStandardItem *lightsItem = new QStandardItem("灯光");
    rootItem->appendRow(lightsItem);

    for (auto model: gRenderer->GetModels()) {
        auto item = new QStandardItem(QString::fromStdString(model->GetName()));
        item->setData(QString::fromStdString(TreeItemModel), Qt::UserRole + TreeItemTypeRoleOffset);
        item->setData(QString::fromStdString(model->GetUUID()), Qt::UserRole + TreeItemUUIDRoleOffset);
        item->setIcon(QIcon("./resource/mesh.png"));
        modelsItem->appendRow(item);
    }

    for (auto light: gRenderer->GetLights()) {
        auto item = new QStandardItem(QString::fromStdString(light->GetName()));
        item->setData(QString::fromStdString(TreeItemLight), Qt::UserRole + TreeItemTypeRoleOffset);
        item->setData(QString::fromStdString(light->GetUUID()), Qt::UserRole + TreeItemUUIDRoleOffset);
        item->setIcon(QIcon("./resource/brightness.png"));
        lightsItem->appendRow(item);
    }
    m_tree_view->expandAll();

    // 设置默认选中
    if (modelsItem->rowCount() > 0) {
        QModelIndex firstModelIndex = modelsItem->child(0)->index();
        m_tree_view->setCurrentIndex(firstModelIndex);

        // 触发点击事件
        QItemSelectionModel *selectionModel = m_tree_view->selectionModel();
        selectionModel->select(firstModelIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        emit m_tree_view->clicked(firstModelIndex);
    }
}
