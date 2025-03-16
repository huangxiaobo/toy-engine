#include "axis.h"
#include <glad/gl.h>
#include "../shader/shader.h"
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "../model/model.h"

Axis::Axis()
{
}

Axis::~Axis()
{

}

void Axis::init(int width, int height)
{
   
}

void Axis::SetModel(Model *model)
{
    m_model = model;    
}

Model* Axis::GetModel()
{
    return m_model;
}

void Axis::Update(long long elapsed)
{
}
