#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <vector>
#include <glm/glm.hpp>

using namespace std;

class Model;
class Axis;
class Light;
class Camera;
class FPSCounter;

class Renderer
{

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
    long long GetModelCount() { return m_models.size(); }
    // 获取全部模型
    std::vector<Model *> GetModels() { return m_models; }
    // 通过名字获取模型
    Model* GetModel(string name);
    // 通过uuid获取模型
    Model* GetModelByUUID(string uuid);

    // 获取全部灯光
    std::vector<Light *> GetLights() { return m_lights; }
    // 通过uuid获取灯光
    Light* GetLightByUUID(std::string uuid);

    // 获取帧率
    float GetFPS();

    // 获取相机
    Camera *GetCamera() { return m_camera; }

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

    FPSCounter *m_fps_counter;
    Axis *m_axis;
    Camera *m_camera;
    Model* m_ground;
    vector<Model *> m_models;
    vector<Light *> m_lights;
};

#endif