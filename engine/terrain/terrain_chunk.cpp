#include "terrain_chunk.h"
#include "noise.h"
#include "../mesh/mesh.h"
#include "../technique/technique_light.h"
#include "../material/material.h"
#include "../light/light.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

/*
 * 构造函数
 *
 * 记录平面参数与噪声生成器，网格在 GenerateMesh() 中一次性生成。
 * 注意：噪声指针由 TerrainManager 持有（不拥有），生命周期由管理器保证。
 */
TerrainChunk::TerrainChunk(float planeSize, int resolution,
                           const Noise* noise, float heightScale)
    : m_planeSize(planeSize)
    , m_resolution(resolution)
    , m_noise(noise)
    , m_heightScale(heightScale)
    , m_technique(nullptr)
    , m_textureID(0) {
}

TerrainChunk::~TerrainChunk() {
    // m_mesh 为 unique_ptr，自动释放
}

/*
 * 设置技术（着色器+材质）
 *
 * 同步更新已生成的网格；若网格尚未生成（先绑定后生成），
 * GenerateMesh() 创建网格时会自动应用已绑定的技术（见 AGENTS.md 经验 1）。
 */
void TerrainChunk::SetTechnique(Technique* tech) {
    m_technique = tech;
    if (m_mesh) {
        m_mesh->SetEffect(tech);
    }
}

/*
 * 设置地形纹理
 *
 * 同步更新已生成的网格；若网格尚未生成，GenerateMesh() 创建网格时
 * 会自动应用已绑定的纹理。
 */
void TerrainChunk::SetTexture(unsigned int textureID) {
    m_textureID = textureID;
    if (m_mesh) {
        m_mesh->SetTexture(textureID);
    }
}

/*
 * 生成网格（幂等：只生成一次）
 *
 * 流程：
 *   1. 生成平面顶点和索引（无高度）
 *   2. 应用噪声高度
 *   3. 计算法线
 *   4. 创建OpenGL网格对象
 *
 * 无 LOD：整个地形只有一个固定分辨率的网格，不存在多级细节切换。
 */
void TerrainChunk::GenerateMesh() {
    // 已生成则跳过
    if (m_mesh) {
        return;
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 1. 生成平面顶点和索引
    GeneratePlaneVertices(vertices, indices);

    // 2. 应用噪声高度
    ApplyHeightToVertices(vertices);

    // 3. 计算法线
    CalculateNormals(vertices, indices);

    // 4. 创建Mesh对象
    Mesh* mesh = new Mesh(vertices, indices);
    mesh->SetDrawMode(GL_TRIANGLES);

    // 设置mesh的着色器效果（若已绑定）
    if (m_technique) {
        mesh->SetEffect(m_technique);
    }

    if (m_textureID != 0) {
        mesh->SetTexture(m_textureID);
    }

    m_mesh.reset(mesh);
}

/*
 * 生成平面网格顶点和索引
 *
 * 顶点布局：
 *   (resolution+1) × (resolution+1) 个顶点
 *   形成 resolution × resolution 个四边形（每个四边形2个三角形）
 *
 * 坐标系：
 *   X轴：水平向右
 *   Z轴：水平向前
 *   Y轴：垂直向上（高度由噪声决定）
 *
 * 平面以世界原点为中心，覆盖 [-planeSize/2, +planeSize/2]。
 */
void TerrainChunk::GeneratePlaneVertices(std::vector<Vertex>& vertices,
                                         std::vector<unsigned int>& indices) const {
    vertices.clear();
    indices.clear();

    int resolution = m_resolution;
    float halfSize = m_planeSize * 0.5f;
    float cellSize = m_planeSize / resolution;

    // 纹理重复次数：每 20 世界单位平铺一次纹理
    // （与原 chunkSize=100、每 chunk 平铺 5 次 的纹理密度保持一致）
    float textureRepeat = m_planeSize / 20.0f;

    // 生成顶点
    for (int z = 0; z <= resolution; z++) {
        for (int x = 0; x <= resolution; x++) {
            // 世界坐标（以原点为中心）
            float worldX = -halfSize + x * cellSize;
            float worldZ = -halfSize + z * cellSize;

            // 纹理坐标（平铺）
            float texX = static_cast<float>(x) / resolution * textureRepeat;
            float texZ = static_cast<float>(z) / resolution * textureRepeat;

            Vertex vertex;
            vertex.Position = glm::vec3(worldX, 0.0f, worldZ);
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.TexCoords = glm::vec2(texX, texZ);
            vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);

            vertices.push_back(vertex);
        }
    }

    // 生成三角形索引
    for (int z = 0; z < resolution; z++) {
        for (int x = 0; x < resolution; x++) {
            unsigned int topLeft = z * (resolution + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * (resolution + 1) + x;
            unsigned int bottomRight = bottomLeft + 1;

            // 第一个三角形（逆时针）
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // 第二个三角形（逆时针）
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

/*
 * 应用噪声高度到顶点
 *
 * 使用FBM（分形布朗运动）生成自然地形
 * 多层噪声叠加产生从大山脉到小岩石的细节层次
 */
void TerrainChunk::ApplyHeightToVertices(std::vector<Vertex>& vertices) const {
    if (!m_noise) return;

    for (auto& vertex : vertices) {
        // 使用世界坐标采样噪声
        float height = m_noise->FBM(
            vertex.Position.x * 0.01f,  // 缩放因子控制地形"密度"
            vertex.Position.z * 0.01f,
            6,      // 6层噪声
            2.0f,   // 频率倍增
            0.5f    // 振幅衰减
        );

        // 映射到 [0, heightScale] 范围（heightScale=0 时地形保持平坦）
        vertex.Position.y = (height * 0.5f + 0.5f) * m_heightScale;
    }
}

/*
 * 计算顶点法线
 *
 * 使用面积加权法：对于每个三角形，计算其面法线
 * 然后将面法线累加到三角形的三个顶点
 * 最后归一化顶点法线
 */
void TerrainChunk::CalculateNormals(std::vector<Vertex>& vertices,
                                    const std::vector<unsigned int>& indices) const {
    // 重置所有法线
    for (auto& v : vertices) {
        v.Normal = glm::vec3(0.0f);
    }

    // 遍历所有三角形
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        glm::vec3& v0 = vertices[i0].Position;
        glm::vec3& v1 = vertices[i1].Position;
        glm::vec3& v2 = vertices[i2].Position;

        // 计算两条边
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        // 面法线（叉积）
        glm::vec3 faceNormal = glm::cross(edge1, edge2);

        // 累加到顶点法线
        vertices[i0].Normal += faceNormal;
        vertices[i1].Normal += faceNormal;
        vertices[i2].Normal += faceNormal;
    }

    // 归一化
    for (auto& v : vertices) {
        v.Normal = glm::normalize(v.Normal);
    }
}

/*
 * 绘制地形网格
 *
 * 流程：
 *   1. 设置着色器和材质
 *   2. 绘制网格
 *
 * 无 LOD：直接绘制唯一的固定分辨率网格，无需按距离切换细节等级。
 */
void TerrainChunk::Draw(long long elapsed,
                        const glm::mat4& projection,
                        const glm::mat4& view,
                        const glm::vec3& cameraPos,
                        const std::vector<Light*>& lights) {
    // 检查网格是否已生成
    if (!m_mesh) {
        return;
    }

    // 设置着色器
    if (m_technique && m_technique->GetType() == TechniqueTypeLight) {
        auto tech = dynamic_cast<TechniqueLight*>(m_technique);
        tech->Enable();
        tech->SetLights(lights);
        tech->SetUniform("gViewPos", cameraPos);

        // 设置投影和视图矩阵
        tech->SetProjectionMatrix(projection);
        tech->SetViewMatrix(view);

        // 模型矩阵（单位矩阵，因为顶点已在世界坐标）
        constexpr auto modelMatrix = glm::mat4(1.0f);
        tech->SetModelMatrix(modelMatrix);
    }

    // 绘制网格
    m_mesh->Draw(elapsed, projection, view, glm::mat4(1.0f), cameraPos, lights);
}