#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <vector>

using namespace std;

class Model;
class Axis;
class Light;

class Renderer 
{


public:
    explicit Renderer();
    virtual ~Renderer();

public:
    void init(int w, int h);
    void draw(long long elapsed);
    void resize(int w, int h);

private:
    int width;
    int height;
    Axis *m_axis;
    vector<Model*> m_models;
    vector<Light*> m_lights;
};

#endif