#ifndef __MODEL_H__
#define __MODEL_H__

#include <QVector3D>
#include <QMatrix4x4>
#include <QVector2D>

 #include "../mesh/mesh.h"
 #include "../technique/technique.h"

class Renderer;
class Technique;
class Mesh;
class Model
{
public:
    QVector<Mesh*> meshes;
    Technique *effect;

    QVector3D position;
    QVector3D Rotation;
    QVector3D Scale;
    QMatrix4x4 matrix;

public:

    Model();
    ~Model();
    void Init();

    void SetMesh(QVector<Mesh *>model);
    void SetEffect(Technique *effect);
    void Draw(long long elapsed, const QMatrix4x4 &projection, const QMatrix4x4 &view, const QMatrix4x4 &model, const QVector3D &camera); 
};

#endif