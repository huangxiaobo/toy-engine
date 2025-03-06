#ifndef __RENDERER_WIDGET_H__
#define __RENDERER_WIDGET_H__
#include <QOpenGLWidget>
// #include <QOpenGLFunctions_4_1_Core>
#include <QElapsedTimer>
#include <QKeyEvent>
#include "axis/axis.h"
// #include "model/model.h"

class Renderer;
class RendererWidget : public QOpenGLWidget //, protected QOpenGLFunctions_4_1_Core
{

    Q_OBJECT

public:
    explicit RendererWidget(QWidget *parent = Q_NULLPTR);
    virtual ~RendererWidget();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    void keyPressEvent( QKeyEvent *e );

private:
    QElapsedTimer m_time;
    Renderer *render;
};

#endif // __RENDERER_WIDGET_H__