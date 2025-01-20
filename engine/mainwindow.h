
#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QWidget>
#include <QMainWindow>

class Renderer;

class ToyEngineMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    ToyEngineMainWindow(QWidget *parent = nullptr);
    ~ToyEngineMainWindow();

protected:
    // void paintEvent(QPaintEvent *event);

private:
    Renderer *renderer;

    QTimer *timer;
};

#endif
