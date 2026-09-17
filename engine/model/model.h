#ifndef __MODEL_H__
#define __MODEL_H__

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../mesh/mesh.h"

class Renderer;
class Mesh;
class Texture;
class Light;
class Material;

class Model {
private:
    std::string m_uuid;
    std::string m_name;
    // Mesh 所有权归模型（unique_ptr 容器，析构自动释放）
    std::vector<std::unique_ptr<Mesh>> m_meshes;
    // 已加载纹理缓存，避免重复加载
    std::vector<Texture> m_textures_loaded;

    glm::vec3 m_position;
    glm::f32 m_rotation;
    glm::vec3 m_scale;
    glm::mat4 m_matrix;

public:
    Model(std::string name);

    virtual ~Model();

    void Init();

    void LoadModel(const std::string &filename);

    void ProcessNode(aiNode *node, const aiScene *scene);

    std::unique_ptr<Mesh> ProcessMesh(aiMesh *mesh, const aiScene *scene);

    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);

    // 批量追加网格（所有权随 unique_ptr 转移给模型）
    void SetMeshes(std::vector<std::unique_ptr<Mesh>> meshes);
    void SetMesh(std::unique_ptr<Mesh> mesh);

    void SetScale(glm::vec3 scale);

    void SetRotate(glm::f32 rotation);

    void SetRotate(glm::f32 rotation, glm::vec3 axis);

    // 平移模型（累积变换：叠加到现有 m_matrix 上）
    void SetTranslate(glm::vec3 position);

    // 设置模型世界位置（一次性重建整个 m_matrix，与 SetTranslate 的累积语义不同）
    void SetPosition(glm::vec3 position);

    const std::vector<std::unique_ptr<Mesh>> &GetMeshes() const;

    /*
     * 获取模型世界变换矩阵
     *
     * 返回与 Draw() 内部完全相同的本地变换矩阵（T(position) × S(scale) × R(m_matrix, rotation)），
     * 供鼠标拾取、日后物理/调试等需要「与渲染几何一致」的空间变换复用。
     * 拾取必须使用本矩阵而非手动重建，否则与画面实际显示的几何位置不一致导致点不中。
     */
    glm::mat4 GetWorldMatrix() const;

    glm::vec3 GetPosition() const;

    glm::vec3 GetScale() const;

    glm::f32 GetRotation() const;

    std::string GetName() const { return m_name; }
    std::string GetUUID() const { return m_uuid; }

    virtual void Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model,
                      const glm::vec3 &camera, const std::vector<Light *> &lights);
};

#endif