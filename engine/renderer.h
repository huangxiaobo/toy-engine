#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <vector>
#include <glm/glm.hpp>

using namespace std;

class Model;
class Axis;
class Light;
class Camera;

class Renderer
{

public:
    explicit Renderer();
    virtual ~Renderer();

public:
    void init(int w, int h);
    void draw(long long elapsed);
    void resize(int w, int h);

public:
    long long GetFrameCount() { return m_frame_count; }

private:
    int width;
    int height;

    glm::mat4 m_projection_matrix;

    Axis *m_axis;
    Camera *m_camera;
    vector<Model *> m_models;
    vector<Light *> m_lights;

    long long m_frame_count;
};

#endif