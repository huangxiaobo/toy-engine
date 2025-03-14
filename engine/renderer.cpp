#include <glad/gl.h>
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
                                           "./resource/shader/default.vert",
                                           "./resource/shader/default.frag");

    axis_model->SetEffect(axis_effect);

    axis_model->SetTranslate(glm::vec3(0, 0.01, 0));
    m_models.push_back(axis_model);

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

    auto model_ground = new ModelGround();
    model_ground->SetScale(glm::vec3(2.1f, 2.0f, 2.1f));
    model_ground->SetMesh(ground_mesh);
    model_ground->SetEffect(ground_effect);
    m_models.push_back(model_ground);

    tinyxml2::XMLDocument doc;
    auto err = doc.LoadFile("./resource/world.xml");
    if (err != tinyxml2::XML_SUCCESS)
    {
        std::cout << "Load failed" << std::endl;
        exit(-1);
    }
    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    std::cout << printer.CStr() << std::endl;

    auto xml_camera = doc.FirstChildElement("world")->FirstChildElement("camera");
    auto camera_position = xml_camera->FirstChildElement("position");
    auto camera_target = xml_camera->FirstChildElement("target");
    m_camera = new Camera(Utils::GetXYZ(camera_position), Utils::GetXYZ(camera_target), glm::vec3(0.0f, 1.0f, 0.0f));

    auto xml_lights = doc.FirstChildElement("world")->FirstChildElement("lights");
    auto xml_light_index = 0;
    for (auto xml_light = xml_lights->FirstChildElement("light"); xml_light != nullptr; xml_light = xml_light->NextSiblingElement())
    {
        auto light_type = xml_light->FirstChildElement("type")->IntText();
        std::cout << "light type: " << light_type << std::endl;
        if (light_type == LightTypePoint)
        {
            auto light = new PointLight();
            light->Color = Utils::GetRGB(xml_light->FirstChildElement("color"));
            light->Position = Utils::GetXYZ(xml_light->FirstChildElement("position"));
            light->AmbientColor = Utils::GetRGB(xml_light->FirstChildElement("ambient")->FirstChildElement("color"));
            light->DiffuseColor = Utils::GetRGB(xml_light->FirstChildElement("diffuse")->FirstChildElement("color"));
            light->SpecularColor = Utils::GetRGB(xml_light->FirstChildElement("specular")->FirstChildElement("color"));
            light->Attenuation.Constant = xml_light->FirstChildElement("attenuation")->FirstChildElement("constant")->FloatText();
            light->Attenuation.Linear = xml_light->FirstChildElement("attenuation")->FirstChildElement("linear")->FloatText();
            light->Attenuation.Exp = xml_light->FirstChildElement("attenuation")->FirstChildElement("exp")->FloatText();
            m_lights.push_back(light);

            // 创建光源模型
            auto model = new ModelPoint(std::format("light-{}", xml_light_index++));
            auto mesh = Mesh::CreatePointMesh(light->Position, light->Color);
            model->SetMesh(mesh);

            Technique *effect = new Technique("light",
                                              "./resource/shader/default.vert",
                                              "./resource/shader/default.frag");

            model->SetEffect(effect);

            m_models.push_back(model);
            light->m_model = model;
            std::cout << "Setup light finish" << std::endl;
        }
    }
    std::cout << "Setup lights finish" << std::endl;

    auto models = doc.FirstChildElement("world")->FirstChildElement("models");
    for (auto model = models->FirstChildElement("model"); model != nullptr; model = model->NextSiblingElement())
    {
        auto model_name = model->FirstChildElement("name")->GetText();

        auto mesh = model->FirstChildElement("mesh");
        auto mesh_name = mesh->Attribute("name");

        auto mesh_file = mesh->FirstChildElement("file")->GetText();
        if (mesh_file == nullptr || std::string(mesh_file).empty())
        {
            // 处理空元素的情况，例如设置一个默认值
            mesh_file = "";
        }

        std::cout << "model name : " << model_name << std::endl;
        std::cout << "mesh name  : " << mesh_name << std::endl;
        std::cout << "mesh file  : " << mesh_file << std::endl;
        if (strcmp(mesh_file, "") == 0)
        {
            continue;
        }
        auto model_obj = new Model(model_name);
        model_obj->LoadModel(mesh_file);

        auto xml_materials = model->FirstChildElement("material");
        Material *material = new Material();
        material->AmbientColor = Utils::GetRGB(xml_materials->FirstChildElement("ambient"));
        material->DiffuseColor = Utils::GetRGB(xml_materials->FirstChildElement("diffuse"));
        material->SpecularColor = Utils::GetRGB(xml_materials->FirstChildElement("specular"));
        material->Shininess = xml_materials->FirstChildElement("shininess")->FloatText();
        model_obj->SetMaterial(material);

        auto xml_shader = model->FirstChildElement("shader");
        auto shader_vert_file = xml_shader->FirstChildElement("vert")->GetText();
        auto shader_frag_file = xml_shader->FirstChildElement("frag")->GetText();

        TechniqueLight *effect = new TechniqueLight(model_name, shader_vert_file, shader_frag_file);
        model_obj->SetEffect(effect);
        effect->SetLights(m_lights);

        m_models.push_back(model_obj);

        auto xml_scale = model->FirstChildElement("scale");
        model_obj->SetScale(Utils::GetXYZ(xml_scale));
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

float Renderer::GetFPS()
{
    return m_fps_counter->GetFPS();
}

void Renderer::calculateProjectMatrix(int w, int h)
{
    float fov = 45.0f;                                                                           // 视野角度
    float aspectRatio = (float)w / (float)(1 * h);                                               // 宽高比
    float nearPlane = 0.1f;                                                                      // 近平面距离
    float farPlane = 100.0f;                                                                     // 远平面距离
    m_projection_matrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane); // 透视
}
