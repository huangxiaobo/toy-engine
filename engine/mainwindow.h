
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

class RendererWidget;
class TreeListView;
class PropertyView;
class QtTreePropertyBrowser;
class QtVariantPropertyManager;
class QtVariantEditorFactory;
class QtProperty;
class QVariant;

enum PropertyType{BOOL_TYPE, STRING_TYPE, INT_TYPE, DOUBLE_TYPE, ENUM_TYPE};


class ToyEngineMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    ToyEngineMainWindow(QWidget *parent = nullptr);
    ~ToyEngineMainWindow();

    void closeEvent(QCloseEvent *event) override;

public slots:
    void onUpdateTreeListView();
    void onTreeListMenuItemClicked();
    void onUpdatePropertyView(QString name);

    void AddProperty(PropertyType type, QString propertyName, bool bEditFlag, QString params);

    void onPropertyChanged(QtProperty *property, const QVariant &value);

    void onMenuOpen();
    void onMenuSave();
    void onMenuAbout();

private:
    RendererWidget *renderer_widget;

    QTimer *timer;
    QStatusBar *m_status_bar;

    // 左侧树形组件
    QTreeView *m_tree_view;
    QStandardItemModel *m_tree_model;
    // 右侧属性组件
    QTreeView *m_property_view;
    QtTreePropertyBrowser* m_property_browser;;
    QtVariantPropertyManager *m_pVarMgrEdit;
    QtVariantPropertyManager *m_pVarMgrOnlyRead;
    QtVariantEditorFactory *m_pVarFactory;
};

#endif
