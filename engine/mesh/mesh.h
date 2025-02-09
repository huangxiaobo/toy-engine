#ifndef __MESH_H__
#define __MESH_H__

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QVector3D>
#include <QVector2D>
#include <QMatrix4x4>
#include <QString>
#include <QVector>
#include <QOpenGLFunctions>

class Vertex
{
public:
    QVector3D Position;
    QVector3D Color;
    QVector3D Normal;
    QVector2D TexCoords;
};

class Mesh
{

public:
    Mesh();
    ~Mesh();
    void SetUpMesh();

    void Draw(long long elapsed);

    static Mesh* CreatePlaneMesh();

public:
    QString name;
    QVector<Vertex> vertices;
    QVector<GLuint> indices;

    GLuint VAO, VBO;
    GLuint EBO; // 创建 EBO 元素缓冲对象
};

#endif