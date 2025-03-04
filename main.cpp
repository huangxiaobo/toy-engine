#include <QApplication>
#include <QWidget>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QSurfaceFormat>

#include "engine/mainwindow.h"

auto main(int argc, char *argv[]) -> int
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    ToyEngineMainWindow w;
    w.show();

    return app.exec();
}