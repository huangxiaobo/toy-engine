#include "model.h"

#include <iostream>
#include <format>

using namespace std;

#include "../mesh/mesh.h"
#include "../texture/texture.h"
#include "../utils/utils.h"
#include "../light/light.h"
#include "../render_context.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

/*
 * 构造函数：初始化模型名称、位置/旋转/缩放默认值，并生成唯一 ID
 *
 * m_matrix 为自定义旋转基准矩阵（默认单位阵），由 SetRotate(rotation, axis) 覆盖写入；
 * 位置/旋转/缩放由 GetWorldMatrix() 从成员变量统一重建，各 setter 均为覆盖语义。
 */
Model::Model(string name) : m_name(name), m_position(0.0f),
                            m_rotation_x(0.0f), m_rotation_y(0.0f), m_rotation_z(0.0f),
                            m_scale(1.0f) {
    m_uuid = Utils::GenerateUUID();
    m_matrix = glm::mat4(1.0f);
}

// 析构：m_meshes 为 unique_ptr 容器，自动释放
Model::~Model() = default;

void Model::Init() {
}

/*
 * 使用 assimp 加载外部模型文件（.obj/.fbx/.gltf 等）
 *
 * 导入参数说明：
 *   - aiProcess_Triangulate        ：将多边形网格三角化，统一为三角形
 *   - aiProcess_FlipUVs            ：翻转 UV 纵轴（assimp 的 UV 原点在左上角，OpenGL 在左下角）
 *   - aiProcess_JoinIdenticalVertices：合并重复顶点，减少顶点数
 *   - aiProcess_GenSmoothNormals   ：缺失法线时生成平滑法线
 *   - aiProcess_CalcTangentSpace   ：计算切线/副切线（用于法线贴图）
 *
 * 加载成功后从根节点递归处理所有子节点几何。
 */
void Model::LoadModel(const string &path) {
    // read file via ASSIMP
    Assimp::Importer importer; // c++接口
    // aiProcess_GenSmoothNormals
    // aiProcess_CalcTangentSpace
    const aiScene *scene = importer.ReadFile(path,
                                             aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
                                             | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
    // 读取数据并做部分处理，返回根节点
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // 检查读取是否正确
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // retrieve the directory path of the filepath
    auto directory = path.substr(0, path.find_last_of('/')); // 保存根目录
    m_directory = directory;

    // process ASSIMP's root node recursively
    ProcessNode(scene->mRootNode, scene); // 处理结点
}

/*
 * 递归处理 assimp 场景节点（场景树遍历）
 *
 * assimp 的场景是节点树：节点不含几何数据，只保存对场景中
 * Mesh 的索引引用。本函数处理当前节点引用的所有 Mesh，
 * 并递归遍历所有子节点，最终把所有几何数据收集到 m_meshes。
 */
void Model::ProcessNode(aiNode *node, const aiScene *scene) {
    std::cout << "ProcessNode: " << node->mNumMeshes << std::endl;
    // process each mesh located at the current node处理当前结点的每个网络
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(ProcessMesh(mesh, scene)); // 递归扫描，将模型数据存储在vector<mesh>中
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes处理当前结点的所有子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene);
    }
}

/*
 * 将单个 assimp aiMesh 转换为引擎 Mesh
 *
 * 数据转换映射：
 *   - 顶点：assimp 的 aiVector3D 逐字段拷贝到 Vertex 结构（assimp 有自己的向量类，不能直接赋值 glm）
 *   - 颜色：当前用顶点位置归一化值作伪色（便于调试，非真实顶点色）
 *   - 法线/切线/副切线：仅在 aiMesh 提供对应数据时才拷贝（assimp 默认不保存，需导入时开启
 *     aiProcess_GenSmoothNormals / aiProcess_CalcTangentSpace）
 *   - 纹理坐标：取第 0 组（一个顶点最多 8 组 UV），缺失时为 (0,0)
 *   - 纹理：按 assimp 材质约定的命名（texture_diffuseN 等）从 .obj 同目录加载贴图
 */
std::unique_ptr<Mesh> Model::ProcessMesh(aiMesh *ai_mesh, const aiScene *scene) {
    // data to fill
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    // 遍历网格的每个顶点
    for (unsigned int i = 0; i < ai_mesh->mNumVertices; i++) // mNumVertices存储了顶点数量
    {
        Vertex vertex; // Vertex结构体在Mesh.h中有定义
        glm::vec3 vector; // 我们声明了一个占位符向量，因为assimp使用它自己的向量类，它不会直接转换到glm的vec3类，所以我们首先将数据转换到这个占位符glm::vec3。
        // positions
        vector.x = ai_mesh->mVertices[i].x;
        vector.y = ai_mesh->mVertices[i].y;
        vector.z = ai_mesh->mVertices[i].z;
        vertex.Position = vector; // 将该点数据存储在结构体中
        // OBJ 不携带顶点色，固定为白色，避免光照链中 albedo 被位置衍生的颜色污染
        vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
        // normals
        if (ai_mesh->HasNormals()) // 同理存储法线（如果有的话）
        {
            vector.x = ai_mesh->mNormals[i].x;
            vector.y = ai_mesh->mNormals[i].y;
            vector.z = ai_mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        // texture coordinates
        if (ai_mesh->mTextureCoords[0]) // 加载纹理坐标
        {
            glm::vec2 vec;
            // 一个顶点最多可以包含8个不同的纹理坐标. 因此，我们假设我们不会
            // 使用顶点可以有多个纹理坐标的模型，所以我们总是取第一个集合(0)。
            vec.x = ai_mesh->mTextureCoords[0][i].x;
            vec.y = ai_mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            // tangent
            vector.x = ai_mesh->mTangents[i].x;
            vector.y = ai_mesh->mTangents[i].y;
            vector.z = ai_mesh->mTangents[i].z;
            vertex.Tangent = vector;
            // bitangent
            vector.x = ai_mesh->mBitangents[i].x;
            vector.y = ai_mesh->mBitangents[i].y;
            vector.z = ai_mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        } else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // 现在遍历网格的每个面(面是网格的三角形)并检索相应的顶点索引。
    for (unsigned int i = 0; i < ai_mesh->mNumFaces; i++) {
        aiFace face = ai_mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            // 每个面都有顶点索引
            indices.push_back(face.mIndices[j]); // 将索引存储在容器中，类似于VEO
        }
    }
    std::cout << "mesh name: " << std::endl;
    std::cout << "indices size: " << indices.size() << std::endl;
    std::cout << "vertices size: " << vertices.size() << std::endl;
    // 处理材料
    aiMaterial *material = scene->mMaterials[ai_mesh->mMaterialIndex];
    // 我们假设在着色器中采样器名称有一个约定。每个漫反射纹理都应该被命名
    // 为'texture_diffuseN'，其中N是一个从1到MAX_SAMPLER_NUMBER的连续数字。
    // 这同样适用于其他纹理，如下列表总结:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // 创建 mesh 并把材质贴图绑定到网格上（漫反射/法线贴图各自取第一张）
    auto mesh = std::make_unique<Mesh>(vertices, indices);
    for (const auto &tex: textures) {
        if (tex.type == "texture_diffuse") {
            mesh->SetTexture(tex.id);
        } else if (tex.type == "texture_normal") {
            mesh->SetNormalMap(tex.id);
        }
    }
    return mesh;
}

// checks all material textures of a given type and loads the textures if they're not loaded yet.
// the required info is returned as a Texture struct.
vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName) {
    vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) // 检查储存在材质中(该类型)纹理的数量
    {
        aiString str;
        mat->GetTexture(type, i, &str); // 获取每个纹理的文件位置，它会将结果储存在一个aiString中
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for (unsigned int j = 0; j < m_textures_loaded.size(); j++) {
            if (std::strcmp(m_textures_loaded[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(m_textures_loaded[j]);
                skip = true;
                // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        if (!skip) {
            // if texture hasn't been loaded already, load it
            Texture texture;
            // 拼出模型目录下的完整路径，加载贴图并得到 GL 纹理 ID
            const std::string resolved = m_directory + "/" + str.C_Str();
            // 【调试】打印 assimp 给出的纹理路径与最终拼接结果，定位黑墙贴图加载失败根因
            std::cout << "[texpath] type=" << typeName
                      << " assimp_path=" << str.C_Str()
                      << " resolved=" << resolved << std::endl;
            texture.id = Utils::LoadTextureFromFile(resolved);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            m_textures_loaded.push_back(texture);
        }
    }
    return textures;
}

/* 追加一个网格到模型（所有权转移） */
void Model::SetMesh(std::unique_ptr<Mesh> mesh) {
    this->m_meshes.push_back(std::move(mesh));
}

/* 批量追加网格到模型（所有权逐项转移） */
void Model::SetMeshes(std::vector<std::unique_ptr<Mesh>> meshes) {
    for (auto &mesh: meshes) {
        this->m_meshes.push_back(std::move(mesh));
    }
}

/* 设置模型缩放：覆盖式更新缩放分量
 *
 * 缩放只记录到成员 m_scale，由 GetWorldMatrix() 统一重建，不再累乘进 m_matrix。
 * 旧实现把新缩放乘进累积矩阵，导致历史缩放无法通过面板还原。 */
void Model::SetScale(glm::vec3 scale) {
    this->m_scale = scale;
}

/* 设置三轴欧拉角旋转（度）：覆盖式写入三个分量
 *
 * 旋转顺序固定为 Y → X → Z（由 GetWorldMatrix 统一重建），
 * 三个分量均为覆盖式设置，与面板/配置的行为一致。 */
void Model::SetRotation(glm::f32 x, glm::f32 y, glm::f32 z) {
    m_rotation_x = x;
    m_rotation_y = y;
    m_rotation_z = z;
}

/* 仅绕 Y 轴旋转：兼容旧接口，等价于 SetRotation(0, rotation, 0) */
void Model::SetRotate(glm::f32 rotation) {
    m_rotation_y = rotation;
}

/* 绕任意轴旋转：覆盖式写入自定义旋转基准矩阵
 *
 * 仅更新 m_matrix（模型空间内的一次旋转），不叠加历史矩阵，
 * 也不改动 m_position/m_rotation_y；绕 Y 轴旋转请用单参版本。 */
void Model::SetRotate(glm::f32 rotation, glm::vec3 axis) {
    m_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), axis);
}

/* 平移模型：覆盖式设置世界位置，等价于 SetPosition */
void Model::SetTranslate(glm::vec3 position) {
    m_position = position;
}

/* 设置模型世界位置：覆盖式，与 SetTranslate 语义一致 */
void Model::SetPosition(glm::vec3 position) {
    m_position = position;
}

/*
 * 模型可动画属性：位置/缩放/旋转三类变换全部支持
 */
bool Model::CanAnimate(AnimProperty prop) const {
    return prop == AnimProperty::Position
        || prop == AnimProperty::Scale
        || prop == AnimProperty::Rotation;
}

/*
 * 写回动画求值结果：按属性枚举分发到自有 setter
 *
 * 动画核心（Animation::Update）只调 SetAnimValue，不知对象具体类型；
 * 这里把"属性 + vec3 值"翻译成模型自己的 SetTranslate/SetScale/SetRotation。
 */
void Model::SetAnimValue(AnimProperty prop, const glm::vec3 &value) {
    switch (prop) {
        case AnimProperty::Position:
            SetTranslate(value);
            break;
        case AnimProperty::Scale:
            SetScale(value);
            break;
        case AnimProperty::Rotation:
            SetRotation(value.x, value.y, value.z);
            break;
        default:
            break;
    }
}

// void Model::SetMaterial(Material *material)
// {
//     this->m_material = material;
// }

// void Model::SetEffect(Technique *effect)
// {
//     this->m_effect = effect;
// }

const std::vector<std::unique_ptr<Mesh>> &Model::GetMeshes() const {
    return m_meshes;
}

glm::vec3 Model::GetPosition() const {
    return m_position;
}

glm::vec3 Model::GetScale() const {
    return m_scale;
}

glm::f32 Model::GetRotationX() const {
    return m_rotation_x;
}

glm::f32 Model::GetRotationY() const {
    return m_rotation_y;
}

glm::f32 Model::GetRotationZ() const {
    return m_rotation_z;
}

glm::f32 Model::GetRotation() const {
    return m_rotation_y;
}

/*
 * 计算模型世界变换矩阵
 *
 * 与 Draw() 保持同一份构建逻辑：T(position) × S(scale) × R(Y→X→Z 欧拉角)，
 * 并把自定义旋转基准 m_matrix 乘到末尾。全部由成员变量覆盖式重建，
 * 保证面板/加载设置的数值与渲染、法线、拾取、包围盒严格一致。
 * 旋转顺序：先绕 Y、再绕 X（局部轴）、最后绕 Z（局部轴）。
 */
glm::mat4 Model::GetWorldMatrix() const {
    auto model_local = glm::mat4(1.0f);
    // 平移：把模型放到世界位置 m_position
    model_local = glm::translate(model_local, m_position);
    // 缩放：按 m_scale 缩放本地几何
    model_local = glm::scale(model_local, m_scale);
    // 三轴欧拉角：Y → X → Z（glm::rotate 逐次左乘，最终 R = Rz·Rx·Ry）
    model_local = glm::rotate(model_local, glm::radians(m_rotation_y), glm::vec3(0.0f, 1.0f, 0.0f));
    model_local = glm::rotate(model_local, glm::radians(m_rotation_x), glm::vec3(1.0f, 0.0f, 0.0f));
    model_local = glm::rotate(model_local, glm::radians(m_rotation_z), glm::vec3(0.0f, 0.0f, 1.0f));
    // 自定义旋转基准（SetRotate(rotation, axis) 写入，默认单位阵）
    model_local = m_matrix * model_local;
    return model_local;
}

/*
 * 绘制模型：构建本地变换矩阵后逐个绘制子网格
 *
 * 变换组合顺序：model * Translate(position) * Scale(scale) * Rotate(Y→X→Z 欧拉角) * m_matrix，
 * 本地变换矩阵统一由 GetWorldMatrix() 生成，与鼠标拾取共用同一份矩阵。
 */
void Model::Draw(const RenderContext &ctx, const glm::mat4 &model) {
    auto model_local = GetWorldMatrix();

    model_local = model * model_local;

    for (int i = 0; i < this->m_meshes.size(); i++) {
        this->m_meshes[i]->Draw(ctx, model_local);
    }
}
