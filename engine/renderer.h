#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

using namespace std;

class Model;
class Axis;
class Light;
class Material;
class Technique;
class Camera;
class FPSCounter;

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
    void LoadWorldFromFile(const string &filename);

    static Camera *LoadCameraFromYaml(const YAML::Node &light_node);

    static Light *LoadLightFromYaml(const YAML::Node &light_node, size_t index);

    static Model *LoadModelFromYaml(const YAML::Node &model_node, size_t index);

    static Material *LoadMaterialFromYaml(const YAML::Node &node, size_t index);

    static Technique *LoadTechniqueFromYaml(const YAML::Node &node, size_t index);

    // 获取模型数量
    int GetModelCount() const { return m_models.size(); }
    // 获取全部模型
    std::vector<Model *> GetModels() { return m_models; }

    // 通过名字获取模型
    Model *GetModel(const string& name);

    // 通过uuid获取模型
    Model *GetModelByUUID(const string& uuid);

    // 获取全部灯光
    std::vector<Light *> GetLights() { return m_lights; }

    // 通过uuid获取灯光
    Light *GetLightByUUID(const std::string &uuid) const;

    // 获取帧率
    float GetFPS() const;

    // 获取相机
    Camera *GetCamera() const { return m_camera; }

    // 切换视角
    void SerProjectionType(ProjectionType type);

    const ProjectionType GetProjectionType() const;

private:
    void calculateProjectMatrix(int w, int h);

private:
    int width;
    int height;

    // 世界矩阵
    glm::mat4 m_projection_matrix;
    glm::mat4 m_view_matrix;
    glm::mat4 m_model_matrix;
    glm::mat4 m_mvp_matrix;
    glm::vec3 m_eye_pos;

    ProjectionType m_projectionType = ProjectionType::Perspective;


    FPSCounter *m_fps_counter;
    Axis *m_axis;
    Camera *m_camera;
    Model *m_ground;
    vector<Model *> m_models;
    map<string, Model *> m_light_models;
    vector<Model *> m_light_modes_ext;
    vector<Light *> m_lights;
};

#endif
