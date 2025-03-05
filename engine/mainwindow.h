
#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QWidget>
#include <QMainWindow>

class RendererWidget;

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
};

#endif
