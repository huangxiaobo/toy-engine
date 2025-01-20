#include <QApplication>
#include <QWidget>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#include "engine/mainwindow.h"

auto main(int argc, char *argv[]) -> int
{
    QApplication app(argc, argv);

    ToyEngineMainWindow w;
    w.show();

    return app.exec();
}