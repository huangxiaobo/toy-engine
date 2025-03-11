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
    long long GetFrameCount() { return m_frame_count; }
    long long GetModelCount(){return m_models.size();}
    std::vector<Model *> GetModels() { return m_models; }
    std::vector<Light *> GetLights() { return m_lights; }
    float GetFPS() ;
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
    vector<Model *> m_models;
    vector<Light *> m_lights;

    long long m_frame_count;
};

#endif