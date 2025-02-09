#include "technique.h"

Technique::Technique(QString name, QString vertexShader, QString fragmentShader)
{
    shaderVertex = vertexShader;
    shaderFragment = fragmentShader;
    shader_program = new QOpenGLShaderProgram();
}

Technique::~Technique()
{
    if (shader_program != nullptr)
    {
        delete shader_program;
        shader_program = nullptr;
    }
}

void Technique::init()
{

    // ===================== 着色器 =====================
    this->shader_program->addShaderFromSourceFile(QOpenGLShader::Vertex, shaderVertex);
    this->shader_program->addShaderFromSourceFile(QOpenGLShader::Fragment, shaderFragment);
    bool success = this->shader_program->link();
    if (!success)
        qDebug() << "ERROR: " << this->shader_program->log();

    this->shader_program->bind();                                              // 如果使用 QShaderProgram，那么最好在获取顶点属性位置前，先 bind()
    ProjectionUniform = this->shader_program->attributeLocation("projection"); // 获取顶点着色器中顶点属性 aPos 的位置
    ViewUniform = this->shader_program->attributeLocation("view");             // 获取顶点着色器中顶点属性 aPos 的位置
    ModelUniform = this->shader_program->attributeLocation("model");           // 获取顶点着色器中顶点属性 aPos 的位置
    WvpUniform = this->shader_program->attributeLocation("gWVP");              // 获取顶点着色器中顶点属性 aPos 的位置
    CameraUniform = this->shader_program->attributeLocation("gViewPos");       // 获取顶点着色器中顶点属性 aPos 的位置
}

void Technique::SetWVP(QMatrix4x4 wvp)
{
    this->shader_program->setUniformValue(WvpUniform, wvp);
}

void Technique::SetCamera(QVector3D camera)
{
    this->shader_program->setUniformValue(CameraUniform, camera);
}

void Technique::SetProjection(QMatrix4x4 projection)
{
    this->shader_program->setUniformValue(ProjectionUniform, projection);
}

void Technique::SetView(QMatrix4x4 view)
{
    this->shader_program->setUniformValue(ViewUniform, view);
}

void Technique::SetModel(QMatrix4x4 model)
{
    this->shader_program->setUniformValue(ModelUniform, model);
}

void Technique::draw(long long elapsed)
{
}

void Technique::setUniform(QString name, QVector3D value)
{
}

void Technique::setUniform(QString name, QVector4D value)
{
}

void Technique::setUniform(QString name, float value)
{
}

void Technique::setUniform(QString name, int value)
{
}

void Technique::setUniform()
{
}

void Technique::Enable()
{
    shader_program->bind();
}

void Technique::Disable()
{
    glUseProgram(0);
}