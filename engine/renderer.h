#ifndef __RENDERER_H__
#define __RENDERER_H__
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class Renderer :public QOpenGLWidget,protected QOpenGLFunctions{

    Q_OBJECT

public:
    explicit Renderer(QWidget *parent = Q_NULLPTR);
    virtual ~Renderer();



protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    
};

#endif