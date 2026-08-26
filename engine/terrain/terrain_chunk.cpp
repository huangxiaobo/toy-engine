#include "terrain_chunk.h"
#include "noise.h"
#include "../mesh/mesh.h"
#include "../technique/technique_light.h"
#include "../material/material.h"
#include "../light/light.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

/*
 * LOD配置表
 *
 * 分辨率：网格顶点数 = resolution × resolution
 * 最大距离：超过此距离切换到下一级LOD
 *
 * 设计原则：
 *   - 相邻LOD的分辨率差2倍（便于网格简化）
 *   - 距离阈值根据视觉效果调整
 */
const LODConfig TerrainChunk::s_lodConfigs[] = {
    {64,  50.0f},   // LOD0: 64×64, 0-50m
    {32,  150.0f},  // LOD1: 32×32, 50-150m
    {16,  300.0f},  // LOD2: 16×16, 150-300m
    {8,   600.0f},  // LOD3: 8×8, 300m+
};

TerrainChunk::TerrainChunk(int x, int z, float chunkSize, int baseResolution,
                           const Noise* noise, float heightScale)
    : chunkX(x)
    , chunkZ(z)
    , m_chunkSize(chunkSize)
    , m_baseResolution(baseResolution)
    , m_noise(noise)
    , m_heightScale(heightScale)
    , m_currentLOD(LODLevel::LOD0)
    , m_technique(nullptr)
    , m_textureID(0) {
}

TerrainChunk::~TerrainChunk() {
}

void TerrainChunk::SetTechnique(Technique* tech) {
    m_technique = tech;
    for (int i = 0; i < static_cast<int>(LODLevel::COUNT); ++i) {
        if (m_lodMeshes[i].generated && m_lodMeshes[i].mesh) {
            m_lodMeshes[i].mesh->SetEffect(tech);
        }
    }
}

void TerrainChunk::SetTexture(unsigned int textureID) {
    m_textureID = textureID;
    for (int i = 0; i < static_cast<int>(LODLevel::COUNT); ++i) {
        if (m_lodMeshes[i].generated && m_lodMeshes[i].mesh) {
            m_lodMeshes[i].mesh->SetTexture(textureID);
        }
    }
}

/*
 * 获取chunk的世界空间中心位置
 * 世界坐标 = chunk坐标 × chunk尺寸
 */
glm::vec3 TerrainChunk::GetWorldPosition() const {
    return glm::vec3(
        chunkX * m_chunkSize,
        0.0f,
        chunkZ * m_chunkSize
    );
}

float TerrainChunk::GetWorldMinX() const {
    return chunkX * m_chunkSize - m_chunkSize * 0.5f;
}

float TerrainChunk::GetWorldMaxX() const {
    return chunkX * m_chunkSize + m_chunkSize * 0.5f;
}

float TerrainChunk::GetWorldMinZ() const {
    return chunkZ * m_chunkSize - m_chunkSize * 0.5f;
}

float TerrainChunk::GetWorldMaxZ() const {
    return chunkZ * m_chunkSize + m_chunkSize * 0.5f;
}

/*
 * 根据摄像机距离计算LOD级别
 *
 * 距离 = 摄像机到chunk中心的水平距离（忽略Y轴）
 * 使用水平距离是因为地形是平面的，Y轴变化不大
 */
LODLevel TerrainChunk::CalculateLOD(const glm::vec3& cameraPos) const {
    glm::vec3 center = GetWorldPosition();
    float dx = cameraPos.x - center.x;
    float dz = cameraPos.z - center.z;
    float distance = std::sqrt(dx * dx + dz * dz);

    // 根据距离选择LOD
    for (int i = 0; i < static_cast<int>(LODLevel::COUNT); i++) {
        if (distance < s_lodConfigs[i].maxDistance) {
            return static_cast<LODLevel>(i);
        }
    }
    return LODLevel::LOD3;  // 最远距离
}

/*
 * 生成指定LOD级别的网格
 *
 * 流程：
 *   1. 生成平面顶点和索引
 *   2. 应用噪声高度
 *   3. 生成裙边（用于LOD缝合）
 *   4. 计算法线
 *   5. 创建OpenGL网格对象
 */
void TerrainChunk::GenerateMesh(LODLevel lod) {
    int lodIndex = static_cast<int>(lod);

    // 如果已生成，跳过
    if (m_lodMeshes[lodIndex].generated) {
        return;
    }

    int resolution = s_lodConfigs[lodIndex].resolution;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 1. 生成平面顶点和索引
    GeneratePlaneVertices(vertices, indices, resolution);

    // 2. 应用噪声高度
    ApplyHeightToVertices(vertices);

    // 3. 生成裙边
    GenerateSkirt(vertices, indices, resolution);

    // 4. 计算法线
    CalculateNormals(vertices, indices);

    // 5. 创建Mesh对象
    Mesh* mesh = new Mesh(vertices, indices);
    mesh->SetDrawMode(GL_TRIANGLES);

    // 设置mesh的着色器效果
    if (m_technique) {
        mesh->SetEffect(m_technique);
    }

    if (m_textureID != 0) {
        mesh->SetTexture(m_textureID);
    }

    m_lodMeshes[lodIndex].mesh = std::unique_ptr<Mesh>(mesh);
    m_lodMeshes[lodIndex].generated = true;
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
 */
void TerrainChunk::GeneratePlaneVertices(std::vector<Vertex>& vertices,
                                          std::vector<unsigned int>& indices,
                                          int resolution) const {
    vertices.clear();
    indices.clear();

    float halfSize = m_chunkSize * 0.5f;
    float cellSize = m_chunkSize / resolution;

    // 世界空间偏移（chunk中心位置）
    float worldOffsetX = chunkX * m_chunkSize;
    float worldOffsetZ = chunkZ * m_chunkSize;

    // 生成顶点
    for (int z = 0; z <= resolution; z++) {
        for (int x = 0; x <= resolution; x++) {
            // 本地坐标 [-halfSize, halfSize]
            float localX = -halfSize + x * cellSize;
            float localZ = -halfSize + z * cellSize;

            // 世界坐标
            float worldX = worldOffsetX + localX;
            float worldZ = worldOffsetZ + localZ;

            // 纹理坐标（平铺）
            float texX = static_cast<float>(x) / resolution * 5.0f;
            float texZ = static_cast<float>(z) / resolution * 5.0f;

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

        // 映射到 [0, heightScale] 范围
        vertex.Position.y = (height * 0.5f + 0.5f) * m_heightScale;
    }
}

/*
 * 生成裙边几何体
 *
 * 裙边的作用：当相邻chunk使用不同LOD时，边界会出现裂缝。
 * 裙边在边界向下延伸一排顶点，视觉上遮住裂缝。
 *
 * 生成方式：
 *   在chunk的四条边上，为每个边界顶点创建一个向下的延伸顶点
 *   然后用三角形连接原始边界和延伸边界
 */
void TerrainChunk::GenerateSkirt(std::vector<Vertex>& vertices,
                                  std::vector<unsigned int>& indices,
                                  int resolution) const {
    float skirtDepth = m_heightScale * 0.5f;  // 裙边深度
    int vertexCount = static_cast<int>(vertices.size());
    int gridSize = resolution + 1;  // 每行顶点数

    // 保存原始边界顶点索引
    std::vector<int> borderIndices;

    // 上边界 (z = 0)
    for (int x = 0; x <= resolution; x++) {
        borderIndices.push_back(x);
    }
    // 右边界 (x = resolution)
    for (int z = 1; z <= resolution; z++) {
        borderIndices.push_back(z * gridSize + resolution);
    }
    // 下边界 (z = resolution)
    for (int x = resolution - 1; x >= 0; x--) {
        borderIndices.push_back(resolution * gridSize + x);
    }
    // 左边界 (x = 0)
    for (int z = resolution - 1; z >= 1; z--) {
        borderIndices.push_back(z * gridSize);
    }

    // 为每个边界顶点创建向下的延伸顶点
    std::vector<int> skirtTopIndices;    // 原始边界顶点
    std::vector<int> skirtBottomIndices; // 延伸顶点

    for (int originalIdx : borderIndices) {
        // 原始顶点
        skirtTopIndices.push_back(originalIdx);

        // 创建延伸顶点（复制原始顶点，Y坐标向下偏移）
        Vertex skirtVertex = vertices[originalIdx];
        skirtVertex.Position.y -= skirtDepth;

        int skirtIdx = static_cast<int>(vertices.size());
        vertices.push_back(skirtVertex);
        skirtBottomIndices.push_back(skirtIdx);
    }

    // 生成裙边三角形
    int borderSize = static_cast<int>(borderIndices.size());
    for (int i = 0; i < borderSize; i++) {
        int next = (i + 1) % borderSize;

        int top0 = skirtTopIndices[i];
        int top1 = skirtTopIndices[next];
        int bottom0 = skirtBottomIndices[i];
        int bottom1 = skirtBottomIndices[next];

        // 两个三角形组成一个四边形
        indices.push_back(top0);
        indices.push_back(bottom0);
        indices.push_back(top1);

        indices.push_back(top1);
        indices.push_back(bottom0);
        indices.push_back(bottom1);
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

        // 紶加到顶点法线
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
 * 绘制chunk
 *
 * 流程：
 *   1. 根据摄像机距离计算LOD级别
 *   2. 如果LOD改变，生成新级别的网格（如果尚未生成）
 *   3. 设置着色器和材质
 *   4. 绘制网格
 */
void TerrainChunk::Draw(long long elapsed,
                         const glm::mat4& projection,
                         const glm::mat4& view,
                         const glm::vec3& cameraPos,
                         const std::vector<Light*>& lights) {
    // 计算当前LOD
    LODLevel newLOD = CalculateLOD(cameraPos);

    // 如果LOD改变，确保新级别的网格已生成
    if (newLOD != m_currentLOD) {
        m_currentLOD = newLOD;
        GenerateMesh(m_currentLOD);
    }

    int lodIndex = static_cast<int>(m_currentLOD);

    // 检查网格是否已生成
    if (!m_lodMeshes[lodIndex].generated || !m_lodMeshes[lodIndex].mesh) {
        return;
    }

    // 设置着色器
    if (m_technique && m_technique->GetType() == TechniqueTypeLight) {
        TechniqueLight* tech = static_cast<TechniqueLight*>(m_technique);
        tech->Enable();
        tech->SetLights(lights);
        tech->SetUniform("gViewPos", cameraPos);

        // 设置投影和视图矩阵
        tech->SetProjectionMatrix(projection);
        tech->SetViewMatrix(view);

        // 模型矩阵（单位矩阵，因为顶点已在世界坐标）
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        tech->SetModelMatrix(modelMatrix);
    }

    // 绘制网格
    m_lodMeshes[lodIndex].mesh->Draw(elapsed, projection, view,
                                       glm::mat4(1.0f), cameraPos, lights);

    // 调试：输出绘制信息（仅第一个chunk）
    if (chunkX == 0 && chunkZ == 0) {
        static int drawCount = 0;
        if (drawCount++ % 60 == 0) {
            std::cout << "Drawing chunk (0,0) LOD" << lodIndex
                      << " vertices: " << m_lodMeshes[lodIndex].mesh->vertices.size()
                      << std::endl;
        }
    }
}

bool TerrainChunk::IsInFrustum(const glm::mat4& viewProjection) const {
    // 简化的视锥体检测：检查chunk中心是否在视锥体内
    // 完整实现需要提取视锥体六个平面并测试AABB
    glm::vec3 center = GetWorldPosition();
    glm::vec4 clipPos = viewProjection * glm::vec4(center, 1.0f);

    // 检查是否在NDC空间内
    return std::abs(clipPos.x) <= clipPos.w &&
           std::abs(clipPos.y) <= clipPos.w &&
           clipPos.z >= 0.0f && clipPos.z <= clipPos.w;
}
