#ifndef __TREELIST_VIEW_H__
#define __TREELIST_VIEW_H__
#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>

class Renderer;

class TreeListView : public QTreeView
{

    Q_OBJECT

public:
    explicit TreeListView(QWidget *parent = Q_NULLPTR);
    virtual ~TreeListView();

    void UpdateList(Renderer* renderer);

public slots:
    void onUpdateTreeListView();

private:
    QStandardItemModel *m_tree_model;
};

#endif