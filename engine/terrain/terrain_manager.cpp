#include "terrain_manager.h"
#include "terrain_chunk.h"
#include "noise.h"
#include "../technique/technique_light.h"
#include "../material/material.h"
#include "../light/light.h"
#include <algorithm>
#include <iostream>
#include <cmath>

TerrainManager::TerrainManager()
    : m_technique(nullptr)
    , m_textureID(0)
    , m_lastCameraChunkX(0)
    , m_lastCameraChunkZ(0) {
}

TerrainManager::~TerrainManager() {
    // 释放所有chunk
    for (auto& [key, chunk] : m_activeChunks) {
        delete chunk;
    }
    m_activeChunks.clear();
}

/*
 * 初始化地形管理器
 *
 * 创建噪声生成器，配置地形参数
 */
void TerrainManager::Init(const glm::vec3& cameraPos, const TerrainConfig& config) {
    m_config = config;

    // 创建噪声生成器
    m_noise = std::make_unique<Noise>();
    m_noise->SetSeed(config.noiseSeed);

    // 初始化摄像机chunk坐标并强制加载初始chunks
    auto [camChunkX, camChunkZ] = WorldToChunkCoords(cameraPos);
    m_lastCameraChunkX = camChunkX;
    m_lastCameraChunkZ = camChunkZ;

    // 加载初始范围内的chunks
    for (int z = camChunkZ - m_config.renderDistance;
         z <= camChunkZ + m_config.renderDistance; z++) {
        for (int x = camChunkX - m_config.renderDistance;
             x <= camChunkX + m_config.renderDistance; x++) {
            int dx = x - camChunkX;
            int dz = z - camChunkZ;
            if (dx * dx + dz * dz <= m_config.renderDistance * m_config.renderDistance) {
                TerrainChunk* chunk = CreateChunk(x, z);
                m_activeChunks[{x, z}] = chunk;
            }
        }
    }

    std::cout << "TerrainManager initialized:" << std::endl;
    std::cout << "  Chunk size: " << config.chunkSize << std::endl;
    std::cout << "  Render distance: " << config.renderDistance << " chunks" << std::endl;
    std::cout << "  Height scale: " << config.heightScale << std::endl;
    std::cout << "  Initial chunks loaded: " << m_activeChunks.size() << std::endl;
}

/*
 * 设置技术（着色器+材质）
 *
 * 将技术应用到所有现有chunk
 * 新创建的chunk也会使用此技术
 */
void TerrainManager::SetTechnique(Technique* tech) {
    m_technique = tech;

    // 应用到所有现有chunk
    for (auto& [key, chunk] : m_activeChunks) {
        chunk->SetTechnique(tech);
    }
}

void TerrainManager::SetTexture(unsigned int textureID) {
    m_textureID = textureID;

    for (auto& [key, chunk] : m_activeChunks) {
        chunk->SetTexture(textureID);
    }

    if (m_technique) {
        m_technique->Enable();
        m_technique->SetUniform("groundTexture", 0);
    }
}

/*
 * 根据世界坐标计算chunk坐标
 *
 * chunk坐标 = floor(世界坐标 / chunk尺寸)
 * 例如：chunkSize=100, 世界坐标(150, -50) → chunk坐标(1, -1)
 */
std::pair<int, int> TerrainManager::WorldToChunkCoords(const glm::vec3& worldPos) const {
    int chunkX = static_cast<int>(std::floor(worldPos.x / m_config.chunkSize));
    int chunkZ = static_cast<int>(std::floor(worldPos.z / m_config.chunkSize));
    return {chunkX, chunkZ};
}

/*
 * 创建新chunk
 *
 * 流程：
 *   1. 创建TerrainChunk对象
 *   2. 生成LOD0（最高精度）网格
 *   3. 设置着色器技术
 *   4. 添加到活跃chunk映射表
 */
TerrainChunk* TerrainManager::CreateChunk(int chunkX, int chunkZ) {
    TerrainChunk* chunk = new TerrainChunk(
        chunkX, chunkZ,
        m_config.chunkSize,
        m_config.baseResolution,
        m_noise.get(),
        m_config.heightScale
    );

    // 设置着色器
    if (m_technique) {
        chunk->SetTechnique(m_technique);
    }

    if (m_textureID != 0) {
        chunk->SetTexture(m_textureID);
    }

    // 预生成LOD0网格
    chunk->GenerateMesh(LODLevel::LOD0);

    return chunk;
}

/*
 * 卸载chunk
 *
 * 从映射表中移除并删除chunk对象
 */
void TerrainManager::UnloadChunk(int chunkX, int chunkZ) {
    auto key = std::make_pair(chunkX, chunkZ);
    auto it = m_activeChunks.find(key);
    if (it != m_activeChunks.end()) {
        delete it->second;
        m_activeChunks.erase(it);
    }
}

/*
 * 更新地形（每帧调用）
 *
 * 核心逻辑：
 *   1. 计算摄像机所在的chunk坐标
 *   2. 在渲染距离范围内加载新chunk
 *   3. 卸载超出范围的chunk
 *   4. 更新每个chunk的LOD级别
 */
void TerrainManager::Update(const glm::vec3& cameraPos, long long elapsed) {
    // 计算摄像机所在的chunk坐标
    auto [camChunkX, camChunkZ] = WorldToChunkCoords(cameraPos);

    // 如果摄像机没有跨越chunk边界，跳过更新（优化）
    if (camChunkX == m_lastCameraChunkX && camChunkZ == m_lastCameraChunkZ) {
        return;
    }

    m_lastCameraChunkX = camChunkX;
    m_lastCameraChunkZ = camChunkZ;

    // 收集需要加载的chunk坐标
    std::vector<std::pair<int, int>> chunksToLoad;
    for (int z = camChunkZ - m_config.renderDistance;
         z <= camChunkZ + m_config.renderDistance; z++) {
        for (int x = camChunkX - m_config.renderDistance;
             x <= camChunkX + m_config.renderDistance; x++) {
            // 使用圆形范围而非方形（可选）
            int dx = x - camChunkX;
            int dz = z - camChunkZ;
            if (dx * dx + dz * dz <= m_config.renderDistance * m_config.renderDistance) {
                chunksToLoad.push_back({x, z});
            }
        }
    }

    // 加载新chunk
    for (const auto& [x, z] : chunksToLoad) {
        auto key = std::make_pair(x, z);
        if (m_activeChunks.find(key) == m_activeChunks.end()) {
            TerrainChunk* chunk = CreateChunk(x, z);
            m_activeChunks[key] = chunk;
        }
    }

    // 卸载超出范围的chunk
    std::vector<std::pair<int, int>> chunksToUnload;
    for (const auto& [key, chunk] : m_activeChunks) {
        int dx = key.first - camChunkX;
        int dz = key.second - camChunkZ;
        if (dx * dx + dz * dz > m_config.unloadDistance * m_config.unloadDistance) {
            chunksToUnload.push_back(key);
        }
    }

    for (const auto& [x, z] : chunksToUnload) {
        UnloadChunk(x, z);
    }

    // 调试输出
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {
        std::cout << "Active chunks: " << m_activeChunks.size()
                  << ", Camera chunk: (" << camChunkX << ", " << camChunkZ << ")"
                  << std::endl;
    }
}

/*
 * 绘制所有可见地形
 *
 * 流程：
 *   1. 遍历所有活跃chunk
 *   2. 对每个chunk进行视锥体剔除
 *   3. 调用chunk的Draw()方法
 */
void TerrainManager::Draw(long long elapsed,
                           const glm::mat4& projection,
                           const glm::mat4& view,
                           const glm::vec3& cameraPos,
                           const std::vector<Light*>& lights) {
    // 计算视图投影矩阵（用于视锥体剔除）
    glm::mat4 viewProjection = projection * view;

    for (auto& [key, chunk] : m_activeChunks) {
        // 视锥体剔除（可选，目前简化实现）
        // if (!chunk->IsInFrustum(viewProjection)) {
        //     continue;
        // }

        chunk->Draw(elapsed, projection, view, cameraPos, lights);
    }
}

/*
 * 获取总三角形数量（用于调试）
 *
 * 遍历所有chunk，累加其当前LOD级别的三角形数
 */
int TerrainManager::GetTotalTriangleCount() const {
    int total = 0;
    for (const auto& [key, chunk] : m_activeChunks) {
        // 简化：假设每个chunk当前LOD的三角形数
        // 实际应该从chunk获取
        total += 64 * 64 * 2;  // 假设LOD0
    }
    return total;
}
