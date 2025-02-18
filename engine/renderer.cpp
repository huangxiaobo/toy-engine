
#include <iostream>
#include "renderer.h"
#include "mesh/mesh.h"
#include "technique/technique.h"
#include "model/model.h"
#include "utils/utils.h"

#include <tinyxml2/tinyxml2.h>
#include <QTime>

Renderer::Renderer(QWidget *parent) : QOpenGLWidget(parent)
{
    this->m_time.start();
}

Renderer::~Renderer()
{
    if (m_axis != nullptr)
    {
        delete m_axis;
    }
    while (!m_models.isEmpty())
    {
        delete m_models.takeFirst();
    }
    if (m_ground != nullptr)
    {
        delete m_ground;
        m_ground = nullptr;
    }
}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();

    const char *version = (const char *)glGetString(GL_VERSION);
    std::cout << "OpenGL version: " << version << std::endl;

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_PROGRAM_POINT_SIZE);

    m_axis = new Axis();
    m_axis->init(width(), height());

    // Create Plane
    QVector<Mesh *> planeMesh = Mesh::CreatePlaneMesh();
    Technique *planeEffect = new Technique("plane",
                                           "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.vert",
                                           "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.frag");
    planeEffect->init();
    Model *plane = new Model();
    plane->SetMesh(planeMesh);
    plane->SetEffect(planeEffect);

    m_models.push_back(plane);

    // Create Ground
    QVector<Mesh *> groundMesh = Mesh::CreateGroundMesh();
    Technique *groundEffect = new Technique("ground",
                                            "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.vert",
                                            "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.frag");
    groundEffect->init();
    m_ground = new Model();
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

    auto models = doc.FirstChildElement("world")->FirstChildElement("models");
    for (auto model = models->FirstChildElement("model"); model != nullptr; model = model->NextSiblingElement())
    {
        auto NameElement = model->FirstChildElement();
        std::cout << "Load success " << NameElement->GetText() << std::endl;
    }
    std::cout << "Setup world finish" << std::endl;
}

void Renderer::paintGL()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    this->m_axis->draw(this->m_time.elapsed());

    QMatrix4x4 mat_projection;
    mat_projection.perspective(50, (float)width() / (float)(1 * height()), 0.1f, 100.0f); // 透视

    QMatrix4x4 mat_model; // QMatrix 默认生成的是一个单位矩阵（对角线上的元素为1）
    QMatrix4x4 mat_view;  // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！

    mat_model.setToIdentity();
    mat_model.scale(5, 5, 5);

    const float radius = 10.0f;
    float time = this->m_time.elapsed() / 1000.0; // 注意是 1000.0
    float cam_x = sin(time) * radius;
    float cam_y = sin(M_PI / 2.0) * radius;
    float cam_z = cos(time) * radius;
    mat_view.lookAt(QVector3D(cam_x, cam_y, cam_z), QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));

    QVector3D cam_pos = QVector3D(cam_x, cam_y, cam_z);

    m_ground->Draw(this->m_time.elapsed(), mat_projection, mat_view, mat_model, cam_pos);
    for (auto model : m_models)
    {
        model->Draw(this->m_time.elapsed(), mat_projection, mat_view, mat_model, cam_pos);
    }
}

void Renderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
