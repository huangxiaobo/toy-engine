#ifndef __TERRAIN_CHUNK_H__
#define __TERRAIN_CHUNK_H__

/*
 * 地形网格（TerrainChunk）
 *
 * 职责：
 *   1. 管理单个地形网格数据（固定分辨率，无 LOD）
 *   2. 根据噪声函数生成程序化高度
 *   3. 计算顶点法线（用于光照）
 *
 * 说明：
 *   - 当前实现不再使用多级 LOD 与裙边缝合，整个地形为单一网格平面
 *   - 平面以世界原点为中心，覆盖 [−planeSize/2, +planeSize/2] × [−planeSize/2, +planeSize/2]
 */

#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Mesh;
class Technique;
class Noise;

class TerrainChunk {
public:
    // 平面参数：planeSize 为平面世界尺寸（长 = 宽），resolution 为每边格子数
    TerrainChunk(float planeSize, int resolution, const Noise* noise, float heightScale);
    ~TerrainChunk();

    // 生成网格（平面顶点 + 噪声高度 + 法线 + OpenGL 缓冲），幂等：只生成一次
    void GenerateMesh();

    // 绘制地形
    void Draw(long long elapsed,
              const glm::mat4& projection,
              const glm::mat4& view,
              const glm::vec3& cameraPos,
              const std::vector<class Light*>& lights);

    // 设置技术（着色器+材质）
    void SetTechnique(Technique* tech);

    void SetTexture(unsigned int textureID);

    // 获取网格三角形数量（用于调试显示）
    int GetTriangleCount() const { return m_resolution * m_resolution * 2; }

private:
    // 生成平面网格（不含高度）
    void GeneratePlaneVertices(std::vector<struct Vertex>& vertices,
                               std::vector<unsigned int>& indices) const;

    // 应用噪声高度到顶点
    void ApplyHeightToVertices(std::vector<struct Vertex>& vertices) const;

    // 计算顶点法线（面积加权法）
    void CalculateNormals(std::vector<struct Vertex>& vertices,
                          const std::vector<unsigned int>& indices) const;

    // 平面参数
    float m_planeSize;      // 平面世界尺寸（长 = 宽）
    int m_resolution;       // 网格分辨率（每边格子数，顶点数 = resolution+1）

    // 噪声与高度
    const Noise* m_noise;   // 噪声生成器（不拥有）
    float m_heightScale;    // 高度缩放因子

    // 地形网格（唯一，无 LOD）
    std::unique_ptr<Mesh> m_mesh;

    // 技术（着色器）
    Technique* m_technique;  // 不拥有

    // 纹理ID
    unsigned int m_textureID;
};

#endif // __TERRAIN_CHUNK_H__