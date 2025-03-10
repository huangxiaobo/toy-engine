#ifndef __RENDERER_WIDGET_H__
#define __RENDERER_WIDGET_H__
#include <QOpenGLWidget>
// #include <QOpenGLFunctions_4_1_Core>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QTimer>
#include <QStatusBar>
#include <QLabel>
#include "axis/axis.h"

class Renderer;
class RendererWidget : public QOpenGLWidget //, protected QOpenGLFunctions_4_1_Core
{

    Q_OBJECT

public:
    explicit RendererWidget(QWidget *parent = Q_NULLPTR);
    virtual ~RendererWidget();

    void updateStatusBar(QStatusBar *status_bar);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    void keyPressEvent( QKeyEvent *e );


private:
    QElapsedTimer m_time;
    Renderer *m_render;


    // 绘制刷新定时器
   QTimer* m_timer;

    // fps显示
    QLabel *m_fps_label;
    QTimer *m_fps_timer;
    int m_frame_count;
};

#endif // __RENDERER_WIDGET_H__