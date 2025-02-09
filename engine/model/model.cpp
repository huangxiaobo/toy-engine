#include "model.h"

#include <QMatrix4x4>
#include "../technique/technique.h"
#include "../mesh/mesh.h"

void Model::Init()
{
}

void Model::SetMesh(Mesh *mesh)
{
    this->mesh = mesh;
}

void Model::SetEffect(Technique *effect)
{
    this->effect = effect;
}

void Model::Draw(long long elapsed, QMatrix4x4 *projection, QMatrix4x4 *view, QMatrix4x4 *model, QVector3D *camera)
{
    QMatrix4x4 mvp = *projection * *view * *model;

    this->effect->Enable();
    this->effect->SetProjection(*projection );
    this->effect->SetView(*view);
    this->effect->SetModel(*model);
    this->effect->SetCamera(*camera );

    this->mesh->Draw(elapsed);

}
