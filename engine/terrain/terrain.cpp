#include "terrain.h"
#include <glad/gl.h>
#include "../model/model.h"
#include "../mesh/mesh.h"
#include "../technique/technique_light.h"
#include "../material/material.h"
#include "../utils/utils.h"

// 使用stb_image加载高度图（实现在utils.cpp中）
#include "../utils/stb_image.h"

Terrain::Terrain() 
    : m_model(nullptr)
    , m_effect(nullptr)
    , m_textureID(0)
    , m_size(10.0f)
    , m_textureRepeat(5) {
}

Terrain::~Terrain() {
    if (m_model != nullptr) {
        delete m_model;
        m_model = nullptr;
    }
    if (m_effect != nullptr) {
        delete m_effect;
        m_effect = nullptr;
    }
}

void Terrain::Init(float size, int textureRepeat) {
    m_size = size;
    m_textureRepeat = textureRepeat;
    
    // 创建带纹理的地面网格
    vector<Mesh *> terrain_mesh = Mesh::CreateTexturedGroundMesh(m_size, m_textureRepeat);
    
    // 创建着色器效果
    m_effect = new Technique("terrain",
                             "./resource/shader/ground.vert",
                             "./resource/shader/ground.frag");
    
    // 创建模型
    m_model = new Model("terrain");
    m_model->SetScale(glm::vec3(2.1f, 2.0f, 2.1f));
    m_model->SetMeshes(terrain_mesh);
    
    // 创建棋盘格纹理 (512x512, 每个格子64像素)
    m_textureID = Utils::CreateCheckerboardTexture(512, 512, 64);
    
    // 为每个网格设置效果和纹理
    for (auto m: terrain_mesh) {
        m->SetEffect(m_effect);
        m->SetTexture(m_textureID);
    }
    
    // 设置纹理uniform
    m_effect->Enable();
    m_effect->SetUniform("groundTexture", 0);
}

void Terrain::InitFromHeightmap(const string &heightmapPath, 
                               float size,
                               float heightScale,
                               int terrainResolution,
                               int textureRepeat) {
    m_heightmapPath = heightmapPath;
    m_size = size;
    m_heightScale = heightScale;
    m_terrainResolution = terrainResolution;
    m_textureRepeat = textureRepeat;
    
    // 从高度图生成地形网格
    vector<Mesh *> terrain_mesh = GenerateTerrainFromHeightmap();
    
    // 创建支持光照的着色器效果
    m_effect = new TechniqueLight("terrain",
                                  "./resource/shader/terrain.vert",
                                  "./resource/shader/terrain.frag");
    
    // 设置材质
    Material *material = new Material();
    material->AmbientColor = glm::vec3(0.3f, 0.3f, 0.3f);
    material->DiffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    material->SpecularColor = glm::vec3(0.5f, 0.5f, 0.5f);
    material->Shininess = 32.0f;
    ((TechniqueLight *)m_effect)->SetMaterial(material);
    
    // 创建模型
    m_model = new Model("terrain");
    m_model->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
    m_model->SetMeshes(terrain_mesh);
    
    // 创建棋盘格纹理
    m_textureID = Utils::CreateCheckerboardTexture(512, 512, 64);
    
    // 为每个网格设置效果和纹理
    for (auto m: terrain_mesh) {
        m->SetEffect(m_effect);
        m->SetTexture(m_textureID);
    }
    
    // 设置纹理uniform
    m_effect->Enable();
    m_effect->SetUniform("groundTexture", 0);
}

void Terrain::Draw(long long elapsed,
                   const glm::mat4 &projection,
                   const glm::mat4 &view,
                   const glm::mat4 &model,
                   const glm::vec3 &camera,
                   const vector<class Light *> &lights) {
    if (m_model != nullptr) {
        // 设置光照
        if (m_effect != nullptr && m_effect->GetType() == TechniqueTypeLight) {
            ((TechniqueLight *)m_effect)->SetLights(lights);
            ((TechniqueLight *)m_effect)->Enable();
            ((TechniqueLight *)m_effect)->SetUniform("gViewPos", camera);
        }
        m_model->Draw(elapsed, projection, view, model, camera, lights);
    }
}

void Terrain::SetScale(glm::vec3 scale) {
    if (m_model != nullptr) {
        m_model->SetScale(scale);
    }
}

void Terrain::SetPosition(glm::vec3 position) {
    if (m_model != nullptr) {
        m_model->SetTranslate(position);
    }
}

vector<Mesh *> Terrain::GenerateTerrainFromHeightmap() {
    vector<Mesh *> meshes;
    
    // 加载高度图
    int width, height, channels;
    unsigned char *data = stbi_load(m_heightmapPath.c_str(), &width, &height, &channels, 1); // 加载为灰度图
    
    if (!data) {
        std::cerr << "Failed to load heightmap: " << m_heightmapPath << std::endl;
        return meshes;
    }
    
    std::cout << "Heightmap loaded: " << width << "x" << height << ", channels: " << channels << std::endl;
    
    // 使用指定的分辨率生成地形网格
    int resolution = m_terrainResolution;
    float stepX = static_cast<float>(width) / resolution;
    float stepZ = static_cast<float>(height) / resolution;
    
    // 计算每个顶点的位置
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    
    float halfSize = m_size / 2.0f;
    float cellSize = m_size / resolution;
    
    // 生成顶点
    for (int z = 0; z <= resolution; z++) {
        for (int x = 0; x <= resolution; x++) {
            // 计算在高度图中的位置
            int imgX = static_cast<int>(x * stepX);
            int imgZ = static_cast<int>(z * stepZ);
            
            // 确保不越界
            imgX = std::min(imgX, width - 1);
            imgZ = std::min(imgZ, height - 1);
            
            // 获取高度值（0-255），归一化到0-1，然后乘以高度缩放
            float heightValue = data[imgZ * width + imgX] / 255.0f;
            float y = heightValue * m_heightScale * 0.0f;
            
            // 计算世界坐标
            float posX = -halfSize + x * cellSize;
            float posZ = -halfSize + z * cellSize;
            
            // 计算纹理坐标
            float texX = static_cast<float>(x) / resolution * m_textureRepeat;
            float texZ = static_cast<float>(z) / resolution * m_textureRepeat;
            
            Vertex vertex;
            vertex.Position = glm::vec3(posX, y, posZ);
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f); // 白色，让纹理显示原色
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); // 暂时设为朝上，后续可以计算真实法线
            vertex.TexCoords = glm::vec2(texX, texZ);
            vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
            
            vertices.push_back(vertex);
        }
    }
    
    // 生成索引
    for (int z = 0; z < resolution; z++) {
        for (int x = 0; x < resolution; x++) {
            unsigned int topLeft = z * (resolution + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * (resolution + 1) + x;
            unsigned int bottomRight = bottomLeft + 1;
            
            // 第一个三角形
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // 第二个三角形
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    std::cout << "Terrain mesh generated: " << vertices.size() << " vertices, " 
              << indices.size() / 3 << " triangles" << std::endl;
    
    // 创建Mesh
    Mesh *mesh = new Mesh(vertices, indices);
    mesh->SetDrawMode(GL_TRIANGLES);
    meshes.push_back(mesh);
    
    // 释放高度图数据
    stbi_image_free(data);
    
    return meshes;
}
