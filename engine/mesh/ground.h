#ifndef __GROUND_H__
#define __GROUND_H__

#include "mesh.h"

class Ground : public Mesh
{
public:
    Ground();
    void Init();
    void Draw(long long elapsed, const QMatrix4x4 &projection, const QMatrix4x4 &view, const QMatrix4x4 &model, const QVector3D &camera);
};

#endif