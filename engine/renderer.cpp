#include <glad/gl.h>
#include "renderer.h"
#include <iostream>

#include "mesh/mesh.h"
#include "technique/technique.h"
#include "technique/technique_light.h"
#include "model/model.h"
#include "axis/axis.h"
#include "utils/utils.h"
#include "light/light.h"
#include "material/material.h"

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
    if (m_ground != nullptr)
    {
        delete m_ground;
        m_ground = nullptr;
    }
    while (!m_lights.empty())
    {
        for (auto light : m_lights)
        {
            delete light;
        }
        m_lights.clear();
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
    std::cout << "OpenGL version: " << version << std::endl;

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    width = w;
    height = h;

    m_axis = new Axis();
    m_axis->init(w, h);

    // Create Plane
    vector<Mesh *> planeMesh = Mesh::CreatePlaneMesh();
    Technique *planeEffect = new Technique("plane",
                                           "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.vert",
                                           "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.frag");
    planeEffect->init();
    Model *plane = new Model("plane");
    plane->SetMesh(planeMesh);
    plane->SetEffect(planeEffect);

    m_models.push_back(plane);

    // Create Ground
    vector<Mesh *> groundMesh = Mesh::CreateGroundMesh();
    Technique *groundEffect = new Technique("ground",
                                            "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.vert",
                                            "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.frag");

    groundEffect->init();
    m_ground = new Model("ground");
    m_ground->SetScale(glm::vec3(2.1f, 2.0f, 2.1f));
    m_ground->SetMesh(groundMesh);
    m_ground->SetEffect(groundEffect);

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

    auto xml_lights = doc.FirstChildElement("world")->FirstChildElement("lights");
    for (auto xml_light = xml_lights->FirstChildElement("light"); xml_light != nullptr; xml_light = xml_light->NextSiblingElement())
    {
        auto light_type = xml_light->FirstChildElement("type")->IntText();
        std::cout << "light type: " << light_type << std::endl;
        if (light_type == 0)
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
        model_obj->material = material;

        auto xml_shader = model->FirstChildElement("shader");
        auto shader_vert_file = xml_shader->FirstChildElement("vert")->GetText();
        auto shader_frag_file = xml_shader->FirstChildElement("frag")->GetText();
        std::cout << "shader vert: " << shader_vert_file << std::endl;
        std::cout << "shader frag: " << shader_frag_file << std::endl;
        TechniqueLight *effect = new TechniqueLight(model_name, shader_vert_file, shader_frag_file);
        effect->init();
        model_obj->SetEffect(effect);

        m_models.push_back(model_obj);

        auto xml_scale = model->FirstChildElement("scale");
        auto scale_x = xml_scale->FirstChildElement("x")->FloatText();
        auto scale_y = xml_scale->FirstChildElement("y")->FloatText();
        auto scale_z = xml_scale->FirstChildElement("z")->FloatText();
        model_obj->SetScale(glm::vec3(scale_x, scale_y, scale_z));
        std::cout << "scale x: " << scale_x << " y: " << scale_y << " z: " << scale_z << std::endl;
    }
    std::cout << "Setup world finish" << std::endl;
}

void Renderer::draw(long long elapsed)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE); //设置 OpenGL 只绘制正面 , 不绘制背面
    // glFrontFace(GL_CW); // 设置顺时针方向 CW : Clock Wind 顺时针方向, 默认是 GL_CCW : Counter Clock Wind 逆时针方向
    glm::mat4 mat_projection;
    float fov = 45.0f;                                                                      // 视野角度
    float aspectRatio = (float)width / (float)(1 * height);                                 // 宽高比
    float nearPlane = 0.1f;                                                                 // 近平面距离
    float farPlane = 100.0f;                                                                // 远平面距离
    mat_projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane); // 透视

    glm::mat4 mat_model; // QMatrix 默认生成的是一个单位矩阵（对角线上的元素为1）
    glm::mat4 mat_view;  // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！

    mat_model = glm::mat4(1.0f);

    const float radius = 10.0f;
    float time = elapsed / 1000.0; // 注意是 1000.0
    float cam_x = sin(time) * radius;
    float cam_y = sin(M_PI / 2.0) * radius;
    float cam_z = cos(time) * radius;
    mat_view = glm::lookAt(glm::vec3(cam_x, cam_y, cam_z), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    glm::vec3 cam_pos = glm::vec3(cam_x, cam_y, cam_z);

    this->m_axis->draw(elapsed, mat_projection, mat_view, mat_model);
    // m_ground->Draw(elapsed, mat_projection, mat_view, mat_model, cam_pos, m_lights);
    for (auto model : m_models)
    {
        model->Draw(elapsed, mat_projection, mat_view, mat_model, cam_pos, m_lights);
    }
}

void Renderer::resize(int w, int h)
{
    width = w;
    height = h;
    glViewport(0, 0, w, h);
}
