#include "terrain_manager.h"
#include "terrain_chunk.h"
#include "noise.h"
#include "../technique/technique_light.h"
#include "../material/material.h"
#include "../light/light.h"
#include <iostream>

TerrainManager::TerrainManager()
    : m_technique(nullptr)
    , m_textureID(0) {
}

TerrainManager::~TerrainManager() {
    // m_plane 为 unique_ptr，自动释放
}

/*
 * 初始化地形管理器
 *
 * 流程：
 *   1. 创建噪声生成器
 *   2. 创建单个平面网格（以世界原点为中心，覆盖 planeSize × planeSize）
 *   3. 应用已绑定的技术与纹理（先绑定后创建的情况）
 *   4. 一次性生成网格
 *
 * 无 LOD：整个地形只有一个固定分辨率的网格，因此不再需要每帧 Update()。
 */
void TerrainManager::Init(const TerrainConfig& config) {
    m_config = config;

    // 创建噪声生成器
    m_noise = std::make_unique<Noise>();
    m_noise->SetSeed(config.noiseSeed);

    // 创建平面网格（单 chunk，无动态加载/卸载）
    m_plane = std::make_unique<TerrainChunk>(
        config.planeSize,
        config.resolution,
        m_noise.get(),
        config.heightScale
    );

    // 应用已提前绑定的技术与纹理（见 AGENTS.md 经验1：先创建后绑定）
    if (m_technique) {
        m_plane->SetTechnique(m_technique);
    }

    if (m_textureID != 0) {
        m_plane->SetTexture(m_textureID);
    }

    // 生成网格（唯一一次，之后无需每帧更新）
    m_plane->GenerateMesh();

    std::cout << "TerrainManager initialized:" << std::endl;
    std::cout << "  Plane size: " << config.planeSize << " x " << config.planeSize << std::endl;
    std::cout << "  Resolution: " << config.resolution << " x " << config.resolution << std::endl;
    std::cout << "  Height scale: " << config.heightScale << std::endl;
    std::cout << "  Noise seed: " << config.noiseSeed << std::endl;
    std::cout << "  Triangles: " << GetTotalTriangleCount() << std::endl;
}

/*
 * 设置技术（着色器+材质）
 *
 * 将技术应用到平面网格（若已生成）；Init() 之后再调用时，
 * SetTechnique 会同步更新已生成的网格（见 AGENTS.md 经验1）。
 */
void TerrainManager::SetTechnique(Technique* tech) {
    m_technique = tech;

    if (m_plane) {
        m_plane->SetTechnique(tech);
    }
}

/*
 * 设置地形纹理
 *
 * 将纹理应用到平面网格（若已生成），并绑定着色器的纹理采样器。
 */
void TerrainManager::SetTexture(unsigned int textureID) {
    m_textureID = textureID;

    if (m_plane) {
        m_plane->SetTexture(textureID);
    }

    if (m_technique) {
        m_technique->Enable();
        m_technique->SetUniform("groundTexture", 0);
    }
}

/*
 * 绘制地形
 *
 * 单一网格平面直接绘制，无需遍历 chunk，
 * 因此不再需要视锥体剔除的多 chunk 遍历。
 */
void TerrainManager::Draw(long long elapsed,
                          const glm::mat4& projection,
                          const glm::mat4& view,
                          const glm::vec3& cameraPos,
                          const std::vector<Light*>& lights) {
    if (m_plane) {
        m_plane->Draw(elapsed, projection, view, cameraPos, lights);
    }
}

/*
 * 获取总三角形数量（用于调试）
 *
 * 平面网格每边 resolution 个格子，每个格子由 2 个三角形组成，
 * 因此总三角形数 = resolution × resolution × 2。
 */
int TerrainManager::GetTotalTriangleCount() const {
    if (!m_plane) {
        return 0;
    }
    return m_plane->GetTriangleCount();
}