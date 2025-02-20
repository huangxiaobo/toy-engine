#include <glad/gl.h>
#include "renderer.h"
#include <iostream>

#include "mesh/mesh.h"
#include "technique/technique.h"
#include "model/model.h"
#include "axis/axis.h"
#include "utils/utils.h"

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
}

void Renderer::init(int w, int h)
{
    // initializeOpenGLFunctions();
    //glad 初始化
    if(!gladLoaderLoadGL())
    {
        std::cout << ("glad init failed!") << std::endl;
        return ;
    }

    const char *version = (const char *)glGetString(GL_VERSION);
    std::cout << "OpenGL version: " << version << std::endl;

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_PROGRAM_POINT_SIZE);

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
    Model *plane = new Model();
    plane->SetMesh(planeMesh);
    plane->SetEffect(planeEffect);

    m_models.push_back(plane);

    // Create Ground
    vector<Mesh *> groundMesh = Mesh::CreateGroundMesh();
    Technique *groundEffect = new Technique("ground",
                                            "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.vert",
                                            "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.frag");

    std::cout << 111 << std::endl;
    groundEffect->init();
    std::cout << 222 << std::endl;
    m_ground = new Model();
    m_ground->SetMesh(groundMesh);
    m_ground->SetEffect(groundEffect);

    /*
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
        if (mesh_file != "")
        {
            auto modelObj = new Model();
            modelObj->LoadModel(mesh_file);
            m_models.push_back(modelObj);
        }
        // if (mesh_file != "") {
        //     QVector<Mesh *> modelMesh = Mesh::CreateMesh(mesh_file);
        //     Technique *modelEffect = new Technique("model",
        //                                            "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.vert",
        //                                            "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.frag");
        //     modelEffect->init();
        //     Model *model = new Model();
        //     model->SetMesh(modelMesh);
        //     model->SetEffect(modelEffect);
        //     m_models.push_back(model);
        // }
    }
    std::cout << "Setup world finish" << std::endl;
    */
}

void Renderer::draw(long long elapsed)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    this->m_axis->draw(elapsed);
    glm::mat4 mat_projection;
    float fov = 45.0f;                                          // 视野角度
    float aspectRatio = (float)width / (float)(1 * height); // 宽高比
    float nearPlane = 0.1f;                                     // 近平面距离
    float farPlane = 100.0f;                                    // 远平面距离
    // glm::perspective(50, (float)width() / (float)(1 * height()), 0.1f, 100.0f);
    mat_projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane); // 透视

    glm::mat4 mat_model; // QMatrix 默认生成的是一个单位矩阵（对角线上的元素为1）
    glm::mat4 mat_view;  // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！

    mat_model = glm::mat4(1.0f);
    mat_model = glm::scale(mat_model, glm::vec3(5.0f, 5.0f, 5.0f));

    const float radius = 10.0f;
    float time = elapsed / 1000.0; // 注意是 1000.0
    float cam_x = sin(time) * radius;
    float cam_y = sin(M_PI / 2.0) * radius;
    float cam_z = cos(time) * radius;
    mat_view = glm::lookAt(glm::vec3(cam_x, cam_y, cam_z), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    glm::vec3 cam_pos = glm::vec3(cam_x, cam_y, cam_z);

    m_ground->Draw(elapsed, mat_projection, mat_view, mat_model, cam_pos);
    for (auto model : m_models)
    {
        model->Draw(elapsed, mat_projection, mat_view, mat_model, cam_pos);
    }
}

void Renderer::resize(int w, int h)
{
    width = w;
    height = h;
    glViewport(0, 0, w, h);
}
