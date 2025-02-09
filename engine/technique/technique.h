#ifndef __TECHNIQUE_H__
#define __TECHNIQUE_H__

#include <QString>
#include <QVector3D>
#include <QOpenGLShaderProgram>
class Technique
{
private:
    QString shaderVertex;
    QString shaderFragment;
    QOpenGLShaderProgram *shader_program;

    GLuint ProjectionUniform;
    GLuint ViewUniform;
    GLuint ModelUniform;
    GLuint WvpUniform; // 模型视图投影矩阵
    GLuint CameraUniform; // 摄像机位置

public:
    Technique(QString name, QString vertexShader, QString fragmentShader);
    ~Technique();

    void init();

    void SetWVP(QMatrix4x4 wvp);
    void SetCamera(QVector3D camera);

    void SetProjection(QMatrix4x4 projection);
    void SetView(QMatrix4x4 view);
    void SetModel(QMatrix4x4 model);

    void draw(long long elapsed);
    void setUniform(QString name, QVector3D value);
    void setUniform(QString name, QVector4D value);
    void setUniform(QString name, float value);
    void setUniform(QString name, int value);
    void setUniform();

    void Enable();

    void Disable();
};

#endif