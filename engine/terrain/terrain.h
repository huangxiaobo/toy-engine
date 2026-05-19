#ifndef __TERRAIN_H__
#define __TERRAIN_H__

#include <glm/glm.hpp>
#include <vector>
#include <string>

using namespace std;

class Model;
class Technique;
class Mesh;

class Terrain {
private:
    Model *m_model;
    Technique *m_effect;
    unsigned int m_textureID;
    
    float m_size;
    int m_textureRepeat;
    
    // 高度图相关
    string m_heightmapPath;
    float m_heightScale;  // 高度缩放因子
    int m_terrainResolution; // 地形网格分辨率

public:
    Terrain();
    ~Terrain();

    // 初始化平坦地形
    void Init(float size = 10.0f, int textureRepeat = 5);
    
    // 从高度图初始化地形
    void InitFromHeightmap(const string &heightmapPath, 
                          float size = 10.0f, 
                          float heightScale = 2.0f,
                          int terrainResolution = 256,
                          int textureRepeat = 5);
    
    void Draw(long long elapsed, 
              const glm::mat4 &projection, 
              const glm::mat4 &view, 
              const glm::mat4 &model,
              const glm::vec3 &camera);
    
    Model* GetModel() const { return m_model; }
    void SetScale(glm::vec3 scale);
    void SetPosition(glm::vec3 position);
    
private:
    // 从高度图生成地形网格
    vector<Mesh *> GenerateTerrainFromHeightmap();
};

#endif
