#include "globals.h"
#include "renderer.h"
#include "model/model.h"

#include "treelist_view.h"

#include <QStandardItem>
#include <QStandardItemModel>


TreeListView::TreeListView(QWidget *parent)
{
    this->setMaximumWidth(200);
    this->setMinimumWidth(150);
    this->setHeaderHidden(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_tree_model = new QStandardItemModel(0, 1, this);
    this->setModel(m_tree_model);
}

TreeListView::~TreeListView()
{
    for (int i = 0; i < m_tree_model->rowCount(); i++)
    {
        delete m_tree_model->item(i);
    }
    m_tree_model->clear();
    delete m_tree_model;
}

void TreeListView::UpdateList(Renderer* renderer)
{
     // 添加数据到 QStandardItemModel
     QStandardItem *rootItem = m_tree_model->invisibleRootItem();
     QStandardItem *parentItem = new QStandardItem("Parent Item");
     rootItem->appendRow(parentItem);
 
     QStandardItem *childItem = new QStandardItem("Child Item");
     parentItem->appendRow(childItem);

     for(auto model : renderer->GetModels()) {
        auto item = new QStandardItem(QString::fromStdString(model->GetName()));
        rootItem->appendRow(item);
    }
}


void TreeListView::onUpdateTreeListView()
{
    QStandardItem *rootItem = m_tree_model->invisibleRootItem();
    QStandardItem *modelsItem = new QStandardItem("模型");
    rootItem->appendRow(modelsItem);

    QStandardItem *lightsItem = new QStandardItem("灯光");
    rootItem->appendRow(lightsItem);

    for(auto model : gRenderer->GetModels()) {
       auto item = new QStandardItem(QString::fromStdString(model->GetName()));
       modelsItem->appendRow(item);
   }
}