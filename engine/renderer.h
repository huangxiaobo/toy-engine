#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <map>
#include <vector>
#include <glm/glm.hpp>
#include "config.h"

using namespace std;

class Model;
class Axis;
class Light;
class Material;
class Technique;
class Camera;
class FPSCounter;
class TerrainManager;
class ParticleSystem;
class SkyDome;

enum class ProjectionType {
    Perspective,
    Orthographic
};

class Renderer {
public:
    explicit Renderer();

    virtual ~Renderer();

public:
    void init(int w, int h);

    void draw(long long elapsed);

    void resize(int w, int h);

    void update(long long elapsed);

public:
    // 获取模型数量
    int GetModelCount() const { return m_models.size(); }
    // 获取全部模型（引用返回，避免每帧拷贝）
    const std::vector<Model *> &GetModels() const { return m_models; }

    // 通过名字获取模型
    Model *GetModel(const string& name);

    // 通过uuid获取模型
    Model *GetModelByUUID(const string& uuid);

    // 获取全部灯光（引用返回，避免每帧拷贝）
    const std::vector<Light *> &GetLights() const { return m_lights; }

    // 通过uuid获取灯光
    Light *GetLightByUUID(const std::string &uuid) const;

    // 获取帧率
    float GetFPS() const;

    // 获取相机
    Camera *GetCamera() const { return m_camera; }
    
    // 获取所有摄像机配置
    const std::vector<Camera*>& GetCameras() const { return m_cameras; }
    
    // 切换到指定索引的摄像机
    void SwitchCamera(int index);

    // 切换视角
    void SerProjectionType(ProjectionType type);

    const ProjectionType GetProjectionType() const;

private:
    void calculateProjectMatrix(int w, int h);

private:
    int width{};
    int height{};

    // 世界矩阵
    glm::mat4 m_projection_matrix{};
    glm::mat4 m_view_matrix{};
    glm::mat4 m_model_matrix{};
    glm::mat4 m_mvp_matrix{};
    glm::vec3 m_eye_pos{};

    ProjectionType m_projectionType = ProjectionType::Perspective;


    FPSCounter *m_fps_counter{};
    Axis *m_axis{};
    Camera* m_camera{};
    vector<Camera *> m_cameras;

    TerrainManager *m_terrain_manager{};
    SkyDome *m_sky_dome{};
    vector<ParticleSystem *> m_particle_systems;
    vector<Model *> m_models;
    map<string, Model *> m_light_models;
    vector<Light *> m_lights;

    // 渲染器创建并拥有的地形纹理，用于退出时统一释放
    unsigned int m_terrain_texture = 0;

    // 渲染器创建并拥有的着色器技术（Technique），用于统一释放
    vector<Technique *> m_techniques;
    // 渲染器创建并拥有的材质（Material），用于统一释放
    vector<Material *> m_materials;
};

#endif
