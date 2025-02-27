#ifndef __MODEL_H__
#define __MODEL_H__

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <vector>
#include <glm/glm.hpp>
#include "../mesh/mesh.h"
// #include "../texture/texture.h"
#include "../technique/technique.h"

using namespace std;

class Renderer;
class Technique;
class Mesh;
class Texture;
class Light;
class Material;

class Model
{
public:
    string Name;
    vector<Mesh *> meshes;
    Material *material;
    Technique *effect;
    vector<Texture> textures_loaded; // 存储到目前为止加载的所有纹理，优化以确保纹理不会加载超过一次。

    glm::vec3 m_position;
    glm::f32 m_rotation;
    glm::vec3 m_scale;
    glm::mat4 m_matrix;

public:
    Model(string name);
    ~Model();
    void Init();

    void LoadModel(const string &filename);
    void ProcessNode(aiNode *node, const aiScene *scene);
    Mesh *ProcessMesh(aiMesh *mesh, const aiScene *scene);

    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName);

    void SetMesh(vector<Mesh *> model);

    void SetScale(glm::vec3 scale);
    void SetRotate(glm::f32 rotation);
    void SetTranslate(glm::vec3 position);
    void SetMaterial(Material *material);
    void SetEffect(Technique *effect);
    void Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::vec3 &camera, vector<Light *> lights);
};

#endif