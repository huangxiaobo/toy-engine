#ifndef __MODEL_H__
#define __MODEL_H__

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../mesh/mesh.h"
#include "../animation/anim_target.h"

class Renderer;
class Mesh;
class Texture;
class Light;
class Material;
struct RenderContext;

class Model : public IAnimTarget {
private:
    std::string m_uuid;
    std::string m_name;
    // 模型文件所在目录，用于拼出贴图相对路径
    std::string m_directory;
    // Mesh 所有权归模型（unique_ptr 容器，析构自动释放）
    std::vector<std::unique_ptr<Mesh>> m_meshes;
    // 已加载纹理缓存，避免重复加载
    std::vector<Texture> m_textures_loaded;

    glm::vec3 m_position;
    // 三轴欧拉角（度）：旋转顺序为 Y → X → Z（依次绕已旋转的局部轴）
    glm::f32 m_rotation_x;
    glm::f32 m_rotation_y;
    glm::f32 m_rotation_z;
    glm::vec3 m_scale;
    // 自定义旋转基准矩阵（SetRotate(rotation, axis) 覆盖写入，默认单位阵）
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

    // 三轴欧拉角旋转（度）：旋转顺序 Y → X → Z
    void SetRotation(glm::f32 x, glm::f32 y, glm::f32 z);

    // 仅绕 Y 轴旋转（兼容旧接口，等价 SetRotation(0, rotation, 0)）
    void SetRotate(glm::f32 rotation);

    void SetRotate(glm::f32 rotation, glm::vec3 axis);

    // 平移模型（覆盖式设置世界位置，等价于 SetPosition）
    void SetTranslate(glm::vec3 position);

    // 设置模型世界位置（覆盖式，与 SetTranslate 语义一致）
    void SetPosition(glm::vec3 position);

    const std::vector<std::unique_ptr<Mesh>> &GetMeshes() const;

    /*
     * 获取模型世界变换矩阵
     *
     * 返回与 Draw() 内部完全相同的本地变换矩阵（T(position) × S(scale) × R(rotation) × m_matrix），
     * 供鼠标拾取、法线收集、包围盒等需要「与渲染几何一致」的空间变换复用。
     * 拾取必须使用本矩阵而非手动重建，否则与画面实际显示的几何位置不一致导致点不中。
     */
    glm::mat4 GetWorldMatrix() const;

    glm::vec3 GetPosition() const;

    glm::vec3 GetScale() const;

    glm::f32 GetRotationX() const;

    // 绕 Y 轴角度（旧接口 GetRotation 的同义返回）
    glm::f32 GetRotationY() const;

    glm::f32 GetRotationZ() const;

    // 兼容旧接口：返回绕 Y 轴角度
    glm::f32 GetRotation() const;

    std::string GetName() const override { return m_name; }
    std::string GetUUID() const { return m_uuid; }

    // ---- IAnimTarget：动画系统通过属性枚举驱动模型变换 ----
    // 支持的属性为：位置/缩放/旋转（模型三类变换全部可动画）
    bool CanAnimate(AnimProperty prop) const override;
    // 写回动画求值结果：位置→SetTranslate，缩放→SetScale，旋转→SetRotation
    void SetAnimValue(AnimProperty prop, const glm::vec3 &value) override;

    virtual void Draw(const RenderContext &ctx, const glm::mat4 &model);
};

#endif