
#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QWidget>
#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
#include <QTimer>
#include <QStatusBar>

class RendererWidget;
class TreeListView;
class ToyEngineMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    ToyEngineMainWindow(QWidget *parent = nullptr);
    ~ToyEngineMainWindow();

    void closeEvent(QCloseEvent *event) override;

private:
    RendererWidget *renderer_widget;

    QTimer *timer;
    QStatusBar *m_status_bar;

    // 左侧树形组件
    TreeListView *m_tree_view;
};

#endif
