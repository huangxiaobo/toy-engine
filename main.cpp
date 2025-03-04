#include <QApplication>
#include <QWidget>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

auto main(int argc, char* argv[]) -> int{
    QApplication app(argc, argv);
    QOpenGLWidget w{/*parent:*/nullptr, /*f:*/Qt::WindowFlags{}};
    w.resize(500,500);
    w.setWindowTitle("Hello OpenGL");
    w.show();
    return app.exec();
}