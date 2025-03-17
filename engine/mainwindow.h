
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

private:
    RendererWidget *renderer_widget;

    QTimer *timer;
    QStatusBar *m_status_bar;

    // 左侧树形组件
    QTreeView *m_tree_view;
    QStandardItemModel *m_tree_model;
    // 右侧属性组件
    QTreeView *m_property_view;
};

#endif
