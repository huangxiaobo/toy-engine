#ifndef __TERRAIN_MANAGER_H__
#define __TERRAIN_MANAGER_H__

/*
 * 地形管理器（TerrainManager）
 *
 * 职责：
 *   1. 管理单个地形网格（固定尺寸平面，无 LOD）
 *   2. 管理噪声生成器、着色器技术与纹理
 *   3. 提供统一的地形绘制接口
 *
 * 说明：
 *   - 旧版本基于 chunk 动态加载/卸载 + 多级 LOD 实现无穷地形；
 *     现简化为单一的固定分辨率网格平面（长 = 宽 = planeSize，以原点为中心）
 *   - 网格在 Init() 时一次性生成，因此不再需要每帧 Update()
 */

#include <glm/glm.hpp>
#include <memory>

class TerrainChunk;
class Technique;
class Noise;
class Light;

// 配置参数
struct TerrainConfig {
    float planeSize = 200.0f;    // 平面世界尺寸（长 = 宽，单位）
    int resolution = 128;        // 网格分辨率（每边格子数，顶点数 = resolution+1）
    float heightScale = 20.0f;   // 地形最大高度（噪声高度缩放因子，0 = 平坦）
    unsigned int noiseSeed = 12345; // 噪声生成器随机种子，相同种子产生相同地形
};

class TerrainManager {
public:
    TerrainManager();
    ~TerrainManager();

    // 初始化地形：创建噪声生成器并生成单个固定分辨率平面网格
    void Init(const TerrainConfig& config = TerrainConfig());

    // 设置技术（着色器+材质）
    void SetTechnique(Technique* tech);

    // 设置地形纹理
    void SetTexture(unsigned int textureID);

    // 绘制地形
    void Draw(long long elapsed,
              const glm::mat4& projection,
              const glm::mat4& view,
              const glm::vec3& cameraPos,
              const std::vector<Light*>& lights);

    // 获取配置
    const TerrainConfig& GetConfig() const { return m_config; }

    // 获取总三角形数量（用于调试）
    int GetTotalTriangleCount() const;

private:
    // 配置
    TerrainConfig m_config;

    // 噪声生成器
    std::unique_ptr<Noise> m_noise;

    // 地形网格（唯一，无 chunk 管理）
    std::unique_ptr<TerrainChunk> m_plane;

    // 技术（着色器）
    Technique* m_technique;  // 不拥有

    // 地形纹理
    unsigned int m_textureID;
};

#endif // __TERRAIN_MANAGER_H__