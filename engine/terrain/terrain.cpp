#include "terrain.h"
#include <glad/gl.h>
#include "../model/model.h"
#include "../mesh/mesh.h"
#include "../technique/technique_light.h"
#include "../material/material.h"
#include "../utils/utils.h"

// stb_image 已经在3rdparty/stb模块中编译，这里只需要包含头文件
#include "stb_image.h"

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

/*
 * 初始化地形（带纹理平面的基础版）
 *
 * 创建带棋盘格纹理的平面网格 + 基础 terrain 着色器，
 * 并设置纹理采样器绑定到纹理单元 0。
 * 注意：此版本为简单平面（无高度起伏），
 * 复杂地形请用 InitFromHeightmap 从高度图生成。
 */
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

/*
 * 从高度图生成地形（带光照的地形版本）
 *
 * 流程：
 *   1. 读取灰度高度图，按 resolution×resolution 顶点网格采样高度
 *   2. 生成地形网格（GenerateTerrainFromHeightmap）
 *   3. 使用支持 Blinn-Phong 光照的 TechniqueLight 着色器并设置材质参数
 *   4. 棋盘格纹理作为地表贴图
 *
 * 参数：
 *   heightmapPath    - 灰度高度图路径（白色=高，黑色=低）
 *   size             - 地形在世界空间的边长
 *   heightScale      - 高度缩放系数
 *   terrainResolution - 网格分辨率（顶点数 = (resolution+1)²）
 *   textureRepeat    - 纹理重复次数
 */
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

/*
 * 绘制地形
 *
 * 若地形使用支持光照的 TechniqueLight，先更新光源列表与摄像机位置
 * uniform（gViewPos 用于 Blinn-Phong 中的视线向量），再交由模型绘制。
 */
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

/* 设置地形在世界空间中的缩放（流转到底层 Model） */
void Terrain::SetScale(glm::vec3 scale) {
    if (m_model != nullptr) {
        m_model->SetScale(scale);
    }
}

/* 设置地形在世界空间中的位置（流转到底层 Model 的平移变换） */
void Terrain::SetPosition(glm::vec3 position) {
    if (m_model != nullptr) {
        m_model->SetTranslate(position);
    }
}

/*
 * 根据高度图生成地形网格（顶点 + 索引）
 *
 * 算法：
 *   1. stbi_load 将高度图按单通道灰度加载（0-255）
 *   2. 采样：把 (resolution+1)² 的网格点映射回高度图像素坐标，
 *      以高度值/255 作为高度（当前乘 0.0f 即保持平坦，仅为预留公式）
 *   3. 世界坐标：以地形中心为原点，x/z 从 -halfSize 到 +halfSize
 *   4. 索引：每个格子拆成 2 个三角形（左上/右下对角线切分）
 *
 * 注意：当前高度 y 乘了 0.0f（保持不变平），如需起伏可去掉 0.0f。
 * 成功返回含一个 Mesh 的 vector；高度图加载失败返回空 vector。
 */
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
