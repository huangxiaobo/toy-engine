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


private:
    Model* model;
};

#endif