#ifndef __TERRAIN_CHUNK_H__
#define __TERRAIN_CHUNK_H__

/*
 * 地形块（TerrainChunk）
 *
 * 职责：
 *   1. 管理单个地形块的网格数据
 *   2. 支持多级LOD（Level of Detail）网格
 *   3. 根据噪声函数生成程序化高度
 *   4. 处理chunk边界的裙边缝合
 *
 * LOD策略：
 *   - LOD0: 最高精度（64×64顶点），近距离使用
 *   - LOD1: 中等精度（32×32顶点），中距离使用
 *   - LOD2: 低精度（16×16顶点），远距离使用
 *   - LOD3: 最低精度（8×8顶点），极远距离使用
 *
 * 裙边（Skirt）：
 *   当相邻chunk使用不同LOD时，边界会出现裂缝。
 *   裙边法在边界向下延伸一排顶点，视觉上遮住裂缝。
 */

#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Mesh;
class Technique;
class Noise;

// LOD级别定义
enum class LODLevel : int {
    LOD0 = 0,  // 最高精度
    LOD1 = 1,  // 中等精度
    LOD2 = 2,  // 低精度
    LOD3 = 3,  // 最低精度
    COUNT = 4
};

// LOD配置参数
struct LODConfig {
    int resolution;      // 网格分辨率（顶点数 = resolution × resolution）
    float maxDistance;    // 切换到此LOD的最大距离
};

class TerrainChunk {
public:
    // chunk坐标（网格坐标，非世界坐标）
    int chunkX;
    int chunkZ;

    TerrainChunk(int x, int z, float chunkSize, int baseResolution,
                 const Noise* noise, float heightScale);
    ~TerrainChunk();

    // 生成指定LOD级别的网格
    void GenerateMesh(LODLevel lod);

    // 绘制chunk
    void Draw(long long elapsed,
              const glm::mat4& projection,
              const glm::mat4& view,
              const glm::vec3& cameraPos,
              const std::vector<class Light*>& lights);

    // 根据距离计算LOD级别
    LODLevel CalculateLOD(const glm::vec3& cameraPos) const;

    // 获取chunk的世界空间中心位置
    glm::vec3 GetWorldPosition() const;

    // 获取chunk的世界空间边界
    float GetWorldMinX() const;
    float GetWorldMaxX() const;
    float GetWorldMinZ() const;
    float GetWorldMaxZ() const;

    // 检查chunk是否在视锥体内（用于剔除）
    bool IsInFrustum(const glm::mat4& viewProjection) const;

    // 设置技术（着色器+材质）
    void SetTechnique(Technique* tech);

    void SetTexture(unsigned int textureID);

private:
    // 生成平面网格（不含高度）
    void GeneratePlaneVertices(std::vector<struct Vertex>& vertices,
                               std::vector<unsigned int>& indices,
                               int resolution) const;

    // 应用噪声高度到顶点
    void ApplyHeightToVertices(std::vector<struct Vertex>& vertices) const;

    // 生成裙边几何体（用于LOD缝合）
    void GenerateSkirt(std::vector<struct Vertex>& vertices,
                       std::vector<unsigned int>& indices,
                       int resolution) const;

    // 计算法线（基于相邻顶点）
    void CalculateNormals(std::vector<struct Vertex>& vertices,
                          const std::vector<unsigned int>& indices) const;

    // LOD配置表
    static const LODConfig s_lodConfigs[];

    // 基础参数
    float m_chunkSize;          // chunk世界尺寸
    int m_baseResolution;       // 基础分辨率
    const Noise* m_noise;       // 噪声生成器（不拥有）
    float m_heightScale;        // 高度缩放因子

    // 各LOD级别的网格
    struct LODMesh {
        std::unique_ptr<Mesh> mesh;
        bool generated = false;
    };
    LODMesh m_lodMeshes[static_cast<int>(LODLevel::COUNT)];

    // 当前绘制的LOD级别
    LODLevel m_currentLOD;

    // 技术（着色器）
    Technique* m_technique;  // 不拥有

    // 纹理ID
    unsigned int m_textureID;
};

#endif // __TERRAIN_CHUNK_H__
