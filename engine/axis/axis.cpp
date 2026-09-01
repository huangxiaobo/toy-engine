#include "axis.h"
#include <glad/gl.h>
#include "../shader/shader.h"
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "../model/model.h"

/*
 * 坐标轴辅助对象（Axis）
 *
 * 在场景中显示世界坐标系三轴（X/Y/Z 方向线段），用于辅助观察空间方位。
 * 实际几何体（线段网格）封装在 Model 中，本类仅持有并管理该 Model 的生命周期。
 */
Axis::Axis()
{
}

Axis::~Axis()
{
    // 释放 Axis 持有的模型
    if (m_model != nullptr) {
        delete m_model;
        m_model = nullptr;
    }
}

// 预留：初始化接口（当前无用，几何体由 SetModel 注入）
void Axis::init(int width, int height)
{
   
}

// 注入坐标轴的网格模型（由 Renderer::init 创建线段网格并传入）
void Axis::SetModel(Model *model)
{
    m_model = model;    
}

Model* Axis::GetModel()
{
    return m_model;
}

// 预留：每帧更新接口（坐标轴静止，无动态更新逻辑）
void Axis::Update(long long elapsed)
{
}