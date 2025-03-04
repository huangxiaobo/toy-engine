#ifndef __RENDERER_H__
#define __RENDERER_H__
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QElapsedTimer>
#include "axis/axis.h"

class Renderer : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core
{

    Q_OBJECT

public:
    explicit Renderer(QWidget *parent = Q_NULLPTR);
    virtual ~Renderer();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    Axis *m_axis;
    QElapsedTimer m_time;
};

#endif