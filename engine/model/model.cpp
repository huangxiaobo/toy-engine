#include "model.h"

#include <iostream>

#include "../technique/technique.h"
#include "../technique/technique_light.h"
#include "../mesh/mesh.h"
#include "../texture/texture.h"
#include "../utils/utils.h"
#include "../light/light.h"
#include "../shader/shader.h"
#include "../material/material.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
Model::Model(string name) : m_name(name), m_position(0.0f), m_rotation(0.0f), m_scale(1.0f)
{
    m_matrix = glm::mat4(1.0f);
}

Model::~Model()
{
    while (!m_meshes.empty())
    {
        delete m_meshes[0];
        m_meshes.erase(m_meshes.begin());
    }
    if (m_effect)
    {
        delete m_effect;
        m_effect = nullptr;
    }
    if (m_material)
    {
        delete m_material;
        m_material = nullptr;
    }
}

void Model::Init()
{
}

// 使用assimp加载模型
void Model::LoadModel(const string &path)
{

    // read file via ASSIMP
    Assimp::Importer importer; // c++接口
    // aiProcess_GenSmoothNormals
    // aiProcess_CalcTangentSpace
    const aiScene *scene = importer.ReadFile(path,
                                             aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace); // 读取数据并做部分处理，返回根节点
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // 检查读取是否正确
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // retrieve the directory path of the filepath
    auto directory = path.substr(0, path.find_last_of('/')); // 保存根目录

    // process ASSIMP's root node recursively
    ProcessNode(scene->mRootNode, scene); // 处理结点
}

void Model::ProcessNode(aiNode *node, const aiScene *scene)
{
    std::cout << "ProcessNode: " << node->mNumMeshes << std::endl;
    // process each mesh located at the current node处理当前结点的每个网络
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(ProcessMesh(mesh, scene)); // 递归扫描，将模型数据存储在vector<mesh>中
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes处理当前结点的所有子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

Mesh *Model::ProcessMesh(aiMesh *ai_mesh, const aiScene *scene)
{
    // data to fill
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    // 遍历网格的每个顶点
    for (unsigned int i = 0; i < ai_mesh->mNumVertices; i++) // mNumVertices存储了顶点数量
    {
        Vertex vertex;    // Vertex结构体在Mesh.h中有定义
        glm::vec3 vector; // 我们声明了一个占位符向量，因为assimp使用它自己的向量类，它不会直接转换到glm的vec3类，所以我们首先将数据转换到这个占位符glm::vec3。
        // positions
        vector.x = ai_mesh->mVertices[i].x;
        vector.y = ai_mesh->mVertices[i].y;
        vector.z = ai_mesh->mVertices[i].z;
        vertex.Position = vector; // 将该点数据存储在结构体中
        vertex.Color = glm::normalize(vector);
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
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // 现在遍历网格的每个面(面是网格的三角形)并检索相应的顶点索引。
    for (unsigned int i = 0; i < ai_mesh->mNumFaces; i++)
    {
        aiFace face = ai_mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {                                        // 每个面都有顶点索引
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

    // return a mesh object created from the extracted mesh data
    Mesh *mesh = new Mesh(vertices, indices);
    return mesh;
}

// checks all material textures of a given type and loads the textures if they're not loaded yet.
// the required info is returned as a Texture struct.
vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
{
    vector<Texture> textures;
    return textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) // 检查储存在材质中(该类型)纹理的数量
    {
        aiString str;
        mat->GetTexture(type, i, &str); // 获取每个纹理的文件位置，它会将结果储存在一个aiString中
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for (unsigned int j = 0; j < m_textures_loaded.size(); j++)
        {
            if (std::strcmp(m_textures_loaded[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(m_textures_loaded[j]);
                skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        // if (!skip)
        // { // if texture hasn't been loaded already, load it
        //     Texture texture;
        //     texture.id = TextureFromFile(str.C_Str(), this->directory); // 将会（用stb_image.h）加载一个纹理并返回该纹理的ID。
        //     texture.type = typeName;
        //     texture.path = str.C_Str();
        //     textures.push_back(texture);        // 在该容器中存储的只有id，类型和路径，对应特定的网络mesh
        //     textures_loaded.push_back(texture); // 将其存储为整个模型加载的纹理，以确保我们不会不必要地加载重复的纹理。
        // }
    }
    return textures;
}

void Model::SetMesh(vector<Mesh *> meshes)
{
    for (auto m : meshes)
    {
        this->m_meshes.push_back(m);
    }
}

void Model::SetScale(glm::vec3 scale)
{
    this->m_scale = scale;
    m_matrix = glm::scale(m_matrix, scale);
}

void Model::SetRotate(glm::f32 rotation)
{
    m_rotation = rotation;
    m_matrix = glm::rotate(m_matrix, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
}

// 绕指定轴旋转
void Model::SetRotate(glm::f32 rotation, glm::vec3 axis)
{
    m_rotation = rotation;
    // 先将模型平移到原点
    glm::mat4 translateToOrigin = glm::translate(glm::mat4(1.0f), -m_position);
    // 进行旋转操作
    glm::mat4 rotate =glm::rotate(glm::mat4(1.0f), glm::radians(rotation), axis);
    // 再将模型平移回原来的位置
    glm::mat4 translateBack = glm::translate(glm::mat4(1.0f), m_position);

    // 组合变换矩阵
    m_matrix = translateBack * rotate * translateToOrigin * m_matrix;
}

void Model::SetTranslate(glm::vec3 position)
{
    m_position = position;
    m_matrix = glm::translate(glm::mat4(1.0f), position);
}

void Model::SetMaterial(Material *material)
{
    this->m_material = material;
}

void Model::SetEffect(Technique *effect)
{
    this->m_effect = effect;
}

void Model::Draw(long long elapsed,
                 const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model,
                 const glm::vec3 &camera, const std::vector<Light *> &lights)
{

    auto model_local = glm::mat4(1.0f);
    model_local = glm::translate(model_local, m_position);
    model_local = glm::scale(model_local, m_scale);
    model_local = glm::rotate(m_matrix, glm::radians(m_rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    model_local = model * model_local;
    glm::mat4 mvp = projection * view * model_local;

    this->m_effect->Enable();
    this->m_effect->SetProjectionMatrix(projection);
    this->m_effect->SetViewMatrix(view);
    this->m_effect->SetModelMatrix(model_local);
    this->m_effect->SetWVPMatrix(mvp);
    this->m_effect->SetCamera(camera);

    this->m_effect->SetLights(lights);
    this->m_effect->SetMaterial(this->m_material);
    this->m_effect->GetShader()->BindFragDataLocation();

    for (int i = 0; i < this->m_meshes.size(); i++)
    {
        this->m_meshes[i]->Draw(elapsed);
    }
}
