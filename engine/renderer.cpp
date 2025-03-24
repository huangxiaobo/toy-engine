#include <glad/gl.h>
#include "globals.h"
#include "config.h"
#include "renderer.h"
#include <iostream>
#include <string>
#include <format>

#include "mesh/mesh.h"
#include "technique/technique.h"
#include "technique/technique_light.h"
#include "model/model.h"
#include "model/model_ground.h"
#include "model/model_point.h"
#include "axis/axis.h"
#include "utils/utils.h"
#include "light/light.h"
#include "material/material.h"
#include "camera/camera.h"
#include "fps/fps.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <tinyxml2/tinyxml2.h>

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
    if (m_camera != nullptr)
    {
        delete m_camera;
        m_camera = nullptr;
    }
    if (m_axis != nullptr)
    {
        delete m_axis;
        m_axis = nullptr;
    }
    while (!m_models.empty())
    {
        for (auto model : m_models)
        {
            delete model;
        }
        m_models.clear();
    }
    while (!m_lights.empty())
    {
        for (auto light : m_lights)
        {
            delete light;
        }
        m_lights.clear();
    }
    if (m_fps_counter != nullptr)
    {
        delete m_fps_counter;
        m_fps_counter = nullptr;
    }
}

void Renderer::init(int w, int h)
{
    // glad 初始化
    if (!gladLoaderLoadGL())
    {
        std::cout << ("glad init failed!") << std::endl;
        return;
    }

    const char *version = (const char *)glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;

    glEnable(GL_DEPTH_TEST);
    // 禁用了程序点大小模式，使用命令指定派生点大小。
    // 如果要启用程序点大小模式，则需要在shader中设置gl_PointSize
    glDisable(GL_PROGRAM_POINT_SIZE);
    // 以填充模式绘制前后
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // 设置 OpenGL 只绘制正面 , 不绘制背面
    // glEnable(GL_CULL_FACE);
    // 设置顺时针方向 CW : Clock Wind 顺时针方向, 默认是 GL_CCW : Counter Clock Wind 逆时针方向
    // glFrontFace(GL_CW);

    width = w;
    height = h;

    m_fps_counter = new FPSCounter();

    m_projection_matrix = glm::mat4(1.0f);
    m_view_matrix = glm::mat4(1.0f);  //  默认生成的是一个单位矩阵（对角线上的元素为1）
    m_model_matrix = glm::mat4(1.0f); // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！
    m_eye_pos = glm::vec3(0, 0, 0);
    calculateProjectMatrix(w, h);

    // 创建坐标系模型
    auto axis_model = new Model("axis");
    auto axis_mesh = Mesh::CreateAxisMesh();
    axis_model->SetMesh(axis_mesh);

    Technique *axis_effect = new Technique("axis",
                                           "./resource/shader/axis.vert",
                                           "./resource/shader/axis.frag");

    axis_model->SetEffect(axis_effect);

    axis_model->SetTranslate(glm::vec3(0, 0.01, 0));
    m_axis = new Axis();
    m_axis->SetModel(axis_model);

    // Create Plane
    vector<Mesh *> plane_mesh = Mesh::CreatePlaneMesh();
    Technique *plane_effect = new Technique("plane",
                                            "./resource/shader/light.vert",
                                            "./resource/shader/light.frag");

    Model *plane = new Model("plane");
    plane->SetScale(glm::vec3(5.0f, 5.0f, 5.0f));
    plane->SetMesh(plane_mesh);
    plane->SetEffect(plane_effect);

    m_models.push_back(plane);

    // Create Ground
    vector<Mesh *> ground_mesh = Mesh::CreateGroundMesh();
    Technique *ground_effect = new Technique("ground",
                                             "./resource/shader/light.vert",
                                             "./resource/shader/light.frag");

    m_ground = new ModelGround();
    m_ground->SetScale(glm::vec3(2.1f, 2.0f, 2.1f));
    m_ground->SetMesh(ground_mesh);
    m_ground->SetEffect(ground_effect);

    m_camera = new Camera(
        gConfig->Camera.Position,
        gConfig->Camera.Target,
        gConfig->Camera.Up);

    int i = 0;
    for (auto lightConfig : gConfig->PointLights)
    {

        auto light = new PointLight(std::format("light-{}", i+1));
        light->Color = lightConfig.Color;
        light->Position = lightConfig.Position;
        light->AmbientColor = lightConfig.AmbientColor;
        light->DiffuseColor = lightConfig.DiffuseColor;
        light->SpecularColor = lightConfig.SpecularColor;
        light->Attenuation.Constant = lightConfig.Attenuation.Constant;
        light->Attenuation.Linear = lightConfig.Attenuation.Linear;
        light->Attenuation.Exp = lightConfig.Attenuation.Exp;
        m_lights.push_back(light);

        // 创建光源模型
        auto model = new ModelPoint(std::format("light-{}", i++));
        // auto mesh = Mesh::CreatePointMesh(light->Position, light->Color);
        auto mesh = Mesh::CreateIcosphereMesh(5, light->Position, light->Color);
        model->SetMesh(mesh);
        model->SetScale(glm::vec3(0.5f));

        Technique *effect = Technique::GetDefaultTechnique();

        model->SetEffect(effect);

        // m_models.push_back(model);
        light->m_model = model;
        std::cout << "Setup light finish" << std::endl;
    }
    std::cout << "Setup lights finish" << std::endl;

    for (auto modelConfig : gConfig->Models)
    {
        std::cout << "model name : " << modelConfig.Name << std::endl;
        std::cout << "mesh name  : " << modelConfig.Mesh.Name << std::endl;
        std::cout << "mesh file  : " << modelConfig.Mesh.File << std::endl;
        if (modelConfig.Mesh.File.length() == 0)
        {
            continue;
        }
        auto model_obj = new Model(modelConfig.Name);
        model_obj->LoadModel(modelConfig.Mesh.File);

        Material *material = new Material();
        material->AmbientColor = modelConfig.Material.AmbientColor;
        material->DiffuseColor = modelConfig.Material.DiffuseColor;
        material->SpecularColor = modelConfig.Material.SpecularColor;
        material->Shininess = modelConfig.Material.Shininess;
        model_obj->SetMaterial(material);

        TechniqueLight *effect = new TechniqueLight(
            "default",
            modelConfig.ShaderVertFile,
            modelConfig.ShaderFragFile);
        model_obj->SetEffect(effect);
        effect->SetLights(m_lights);

        model_obj->SetScale(modelConfig.Scale);
        model_obj->SetTranslate(modelConfig.Position);
        model_obj->SetRotate(modelConfig.Rotation);

        m_models.push_back(model_obj);
    }
    std::cout << "Setup world finish" << std::endl;
}

void Renderer::draw(long long elapsed)
{
    // 绘制帧数量加1
    m_fps_counter->Add();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_view_matrix = m_camera->GetViewMatrix(); // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！
    m_eye_pos = m_camera->GetEyePosition();

    m_axis->GetModel()->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights); // 【重点】 绘制坐标轴

    m_ground->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);

    for (auto light : m_lights)
    {
        light->GetModel()->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }

    for (auto model : m_models)
    {
        model->Draw(elapsed, m_projection_matrix, m_view_matrix, m_model_matrix, m_eye_pos, m_lights);
    }
}

void Renderer::resize(int w, int h)
{
    width = w;
    height = h;
    calculateProjectMatrix(w, h);
    glViewport(0, 0, w, h);
}

void Renderer::update(long long elapsed)
{

    m_eye_pos = m_camera->GetEyePosition();

    // 更新灯光
    for (auto light : m_lights)
    {
        if (light->GetLightType() == LightTypePoint)
        {
            auto point_light = (PointLight *)light;
            point_light->Position = glm::vec3(glm::rotate(glm::radians(0.5f), glm::vec3(0, 1, 0)) * glm::vec4(point_light->Position, 1.0f));
            point_light->m_model->SetTranslate(point_light->Position);
        }
    }
}

const Model *Renderer::GetModel(string name)
{
    for (auto model : m_models)
    {
        if (model->GetName() == name)
        {
            return model;
        }
    }
    return nullptr;
}

const Model *Renderer::GetModelByUUID(string uuid)
{ for (auto model : m_models)
    {
        if (model->GetUUID() == uuid)
        {
            return model;
        }
    }
    return nullptr;
}

float Renderer::GetFPS()
{
    return m_fps_counter->GetFPS();
}

void Renderer::calculateProjectMatrix(int w, int h)
{
    float fov = gConfig->Clip.ClipFov;                                                           // 视野角度
    float aspectRatio = (float)w / (float)(1 * h);                                               // 宽高比
    float nearPlane = gConfig->Clip.ClipNear;                                                    // 近平面距离
    float farPlane = gConfig->Clip.ClipFar;                                                      // 远平面距离
    m_projection_matrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane); // 透视
}
