
#include <iostream>
#include "renderer.h"
#include "mesh/mesh.h"
#include "technique/technique.h"
#include "model/model.h"
#include "utils/utils.h"
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
    Mesh *planeMesh = Mesh::CreatePlaneMesh();
    Technique *planeEffect = new Technique("plane",
                                           "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.vert",
                                           "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/light.frag");
    planeEffect->init();
    Model *plane = new Model();
    plane->SetMesh(planeMesh);
    plane->SetEffect(planeEffect);

    m_models.push_back(plane);
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
    float cam_z = cos(time) * radius;
    mat_view.lookAt(QVector3D(cam_x, 0.0f, cam_z), QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));

    QVector3D cam_pos = QVector3D(cam_x, 0.0f, cam_z);

    for (auto model : m_models)
    {
        model->Draw(this->m_time.elapsed(), mat_projection, mat_view, mat_model, cam_pos);
    }
}

void Renderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
