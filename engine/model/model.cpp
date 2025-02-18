#include "model.h"

#include <iostream>
#include <QMatrix4x4>

#include "../technique/technique.h"
#include "../mesh/mesh.h"

Model::Model()
{
}

Model::~Model()
{
    while (!meshes.empty()) {
        delete meshes[0];
        meshes.erase(meshes.begin());
    }
    if (effect) {
        delete effect;
        effect = nullptr;
    }
}

void Model::Init()
{
   
}

void Model::SetMesh(QVector<Mesh*> meshes)
{
    for(auto m : meshes) {
        this->meshes.push_back(m);
    }
}

void Model::SetEffect(Technique *effect)
{
    this->effect = effect;
}

void Model::Draw(long long elapsed, const QMatrix4x4 &projection, const QMatrix4x4 &view, const QMatrix4x4 &model, const QVector3D &camera)
{
    QMatrix4x4 mvp = projection * view * model;

    this->effect->Enable();
    this->effect->SetProjection(projection);
    this->effect->SetView(view);
    this->effect->SetModel(model);
    this->effect->SetCamera(camera);

    for(int i = 0; i < this->meshes.size(); i++) {
        this->meshes[i]->Draw(elapsed);
    }
}
