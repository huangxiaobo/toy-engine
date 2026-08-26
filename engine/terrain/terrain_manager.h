#ifndef __TERRAIN_MANAGER_H__
#define __TERRAIN_MANAGER_H__

/*
 * 地形管理器（TerrainManager）
 *
 * 职责：
 *   1. 管理地形块（TerrainChunk）的生命周期
 *   2. 根据摄像机位置动态加载/卸载chunk
 *   3. 提供统一的地形绘制接口
 *   4. 管理噪声生成器和材质
 *
 * 工作流程：
 *   1. 每帧调用Update()，根据摄像机位置确定需要加载的chunk
 *   2. 新chunk在需要时创建并生成网格
 *   3. 超出范围的chunk被卸载以释放内存
 *   4. 绘制时遍历所有活跃chunk并调用其Draw()
 *
 * 内存管理：
 *   - 使用unordered_map存储活跃chunk
 *   - chunk坐标(x,z)作为key
 *   - 超出卸载距离的chunk自动删除
 */

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <memory>

class TerrainChunk;
class Technique;
class Noise;
class Light;

// 配置参数
struct TerrainConfig {
    float chunkSize = 100.0f;
    int baseResolution = 64;
    int renderDistance = 5;
    int unloadDistance = 7;
    float heightScale = 20.0f;
    int noiseOctaves = 6;
    float noiseFrequency = 0.01f;
    unsigned int noiseSeed = 12345;
};

// chunk坐标哈希函数（用于unordered_map）
struct ChunkKeyHash {
    size_t operator()(const std::pair<int, int>& key) const {
        // 使用质数哈希组合
        size_t h1 = std::hash<int>()(key.first);
        size_t h2 = std::hash<int>()(key.second);
        return h1 ^ (h2 << 1);
    }
};

class TerrainManager {
public:
    TerrainManager();
    ~TerrainManager();

    // 初始化地形管理器
    void Init(const glm::vec3& cameraPos, const TerrainConfig& config = TerrainConfig());

    // 更新地形（每帧调用）
    // 根据摄像机位置加载/卸载chunk
    void Update(const glm::vec3& cameraPos, long long elapsed);

    // 绘制所有可见地形
    void Draw(long long elapsed,
              const glm::mat4& projection,
              const glm::mat4& view,
              const glm::vec3& cameraPos,
              const std::vector<Light*>& lights);

    // 设置技术（着色器+材质）
    void SetTechnique(Technique* tech);

    // 设置地形纹理
    void SetTexture(unsigned int textureID);

    // 获取配置
    const TerrainConfig& GetConfig() const { return m_config; }

    // 获取活跃chunk数量（用于调试）
    int GetActiveChunkCount() const { return static_cast<int>(m_activeChunks.size()); }

    // 获取总三角形数量（用于调试）
    int GetTotalTriangleCount() const;

private:
    // 根据世界坐标计算chunk坐标
    std::pair<int, int> WorldToChunkCoords(const glm::vec3& worldPos) const;

    // 创建新chunk
    TerrainChunk* CreateChunk(int chunkX, int chunkZ);

    // 卸载chunk
    void UnloadChunk(int chunkX, int chunkZ);

    // 配置
    TerrainConfig m_config;

    // 噪声生成器
    std::unique_ptr<Noise> m_noise;

    // 活跃chunk映射表
    // key: chunk坐标(x, z), value: TerrainChunk*
    std::unordered_map<std::pair<int, int>, TerrainChunk*, ChunkKeyHash> m_activeChunks;

    // 技术（着色器）
    Technique* m_technique;  // 不拥有

    // 地形纹理
    unsigned int m_textureID;

    // 上一帧的摄像机chunk坐标（用于检测是否需要更新）
    int m_lastCameraChunkX;
    int m_lastCameraChunkZ;
};

#endif // __TERRAIN_MANAGER_H__
