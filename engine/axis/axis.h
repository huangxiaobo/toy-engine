#ifndef __AXIS_H__
#define __AXIS_H__

#include <glm/glm.hpp>

class Model;

class Axis
{
public:
    Axis();
    ~Axis();

    void init(int width, int height);

public:

    void SetModel(Model* model);
    Model* GetModel();
    void Update(long long elapsed);


private:
    Model* m_model;
};

#endif