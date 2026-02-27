#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QWidget>
#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
#include <QTimer>
#include <QStatusBar>
#include <QTreeView>
#include <QString>
//#include <QtVariantEditorFactory>
#include <QUuid>
#include <QHash>

using namespace std;

class RendererWidget;
class TreeListView;
class PropertyView;
class QtTreePropertyBrowser;
class QtVariantPropertyManager;
class QtVariantEditorFactory;
class QtProperty;
class QVariant;

enum PropertyType {
    BOOL_TYPE,
    STRING_TYPE,
    INT_TYPE,
    DOUBLE_TYPE,
    ENUM_TYPE
};

enum PropertyLabel {
    PROPERTY_LABEL_NONE,
    PROPERTY_LABEL_MODEL_POSITION_X,
    PROPERTY_LABEL_MODEL_POSITION_Y,
    PROPERTY_LABEL_MODEL_POSITION_Z,
    PROPERTY_LABEL_MODEL_SCALE_X,
    PROPERTY_LABEL_MODEL_SCALE_Y,
    PROPERTY_LABEL_MODEL_SCALE_Z,
    PROPERTY_LABEL_MODEL_ROTATION,
    PROPERTY_LABEL_LIGHT_COLOR_RGBA,
    PROPERTY_LABEL_LIGHT_AMBIENT,
    PROPERTY_LABEL_LIGHT_SPECULAR,
    PROPERTY_LABEL_LIGHT_DIFFUSE,
};

class ToyEngineMainWindow : public QMainWindow {
    Q_OBJECT

public:
    ToyEngineMainWindow(QWidget *parent = nullptr);

    ~ToyEngineMainWindow();

    void closeEvent(QCloseEvent *event) override;

public slots:
    void onUpdateTreeListView();

    void onTreeListMenuItemClicked();

    void onUpdatePropertyView(QString name);

    void onMenuAbout();

private:
    void InitMenuBar();

    void InitStatusBar();

    void InitPropertyViewOfLight(const string &uuid);

    void onLightPropertyChanged(const string &uuid, QtProperty *property, const QVariant &value);

    void InitPropertyViewOfModel(const string& uuid);

    void onModelPropertyChanged(const string& modelUUID, QtProperty *property, const QVariant &value);


private:
    RendererWidget *m_renderer_widget;

    QTimer *timer;
    QMenuBar *m_menu_bar;
    QStatusBar *m_status_bar;

    // 左侧树形组件
    QTreeView *m_tree_view;
    QStandardItemModel *m_tree_model;

    // QTreeView *m_property_view;
    QtTreePropertyBrowser *m_property_browser;
    ;
    QtVariantPropertyManager *m_var_prop_mgr_edit;
    QHash<QUuid, QMetaObject::Connection> m_connections{};
    QtVariantPropertyManager *m_var_prop_mgr_onlyread;
    QtVariantEditorFactory *m_var_edit_factory;
    std::map<QtProperty *, PropertyLabel> m_property_label_map;
};

#endif
