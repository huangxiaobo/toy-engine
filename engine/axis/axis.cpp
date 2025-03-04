#include "axis.h"

#include <QOpenGLVertexArrayObject>
#include <iostream>
Axis::Axis()
{
    shader_program = new QOpenGLShaderProgram();
}

Axis::~Axis()
{
    /* 对象的回收 */
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Axis::init(int width, int height)
{
    float vertices[] = {
        // texture coords
        0.5f, 0.5f, 0.0f,   // top right
        0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f   // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    // ===================== 着色器 =====================
    this->shader_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/axis.vert");
    this->shader_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "/Users/huangxiaobo/Workspace/github.com@huangxiaobo/toy-engine/resource/shader/axis.frag");
    bool success = this->shader_program->link();
    if (!success)
        qDebug() << "ERROR: " << this->shader_program->log();

    // ===================== VAO | VBO =====================
    // VAO 和 VBO 对象赋予 ID
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 绑定 VAO、VBO 对象
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    /* 为当前绑定到 target 的缓冲区对象创建一个新的数据存储（在 GPU 上创建对应的存储区域，并将内存中的数据发送过去）
        如果 data 不是 NULL，则使用来自此指针的数据初始化数据存储
        void glBufferData(GLenum target,  // 需要在 GPU 上创建的目标
                                            GLsizeipter size,  // 创建的显存大小
                                            const GLvoid* data,  // 数据
                                            GLenum usage)  // 创建在 GPU 上的哪一片区域（显存上的每个区域的性能是不一样的）https://registry.khronos.org/OpenGL-Refpages/es3.0/
    */
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

#if 1
    /* 告知显卡如何解析缓冲区里面的属性值
        void glVertexAttribPointer(
                                    GLuint index,  // VAO 中的第几个属性（VAO 属性的索引）
                                    GLint size,  // VAO 中的第几个属性中对应的位置放几份数据
                                    GLEnum type,  // 存放数据的数据类型
                                    GLboolean normalized,  // 是否标准化
                                    GLsizei stride,  // 步长
                                    const void* offset  // 偏移量
        )
    */
    this->shader_program->bind();                                                             // 如果使用 QShaderProgram，那么最好在获取顶点属性位置前，先 bind()
    GLint aPosLocation = this->shader_program->attributeLocation("position");                 // 获取顶点着色器中顶点属性 aPos 的位置
    glVertexAttribPointer(aPosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0); // 手动传入第几个属性
    glEnableVertexAttribArray(aPosLocation);                                                  // 开始 VAO 管理的第一个属性值
#endif

#if 0
    /* 当我们在顶点着色器中没有写 layout 时，也可以在此处代码根据名字手动指定某个顶点属性的位置 */
    this->shader_program->bind();
    GLint aPosLocation = 2;
    this->shader_program->bindAttributeLocation("position", aPosLocation);
    glVertexAttribPointer(aPosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(aPosLocation);

#endif

    // ===================== EBO =====================
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // EBO/IBO 是储存顶点【索引】的

    // 解绑 VAO 和 VBO，注意先解绑 VAO再解绑EBO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // 注意 VAO 不参与管理 VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    QMatrix4x4 mat_projection;
    mat_projection.perspective(50, (float)width / (float)(1 * height), 0.1f, 100.0f); // 透视
    this->shader_program->setUniformValue("mat_projection", mat_projection);
}

void Axis::draw(long long elapsed)
{
    QMatrix4x4 mat_model; // QMatrix 默认生成的是一个单位矩阵（对角线上的元素为1）
    QMatrix4x4 mat_view;  // 【重点】 view代表摄像机拍摄的物体，也就是全世界！！！

    mat_model.setToIdentity();
    mat_model.scale(5, 5, 5);

    const float radius = 10.0f;
    float time = elapsed / 1000.0; // 注意是 1000.0
    float cam_x = sin(time) * radius;
    float cam_z = cos(time) * radius;
    mat_view.lookAt(QVector3D(cam_x, 0.0f, cam_z), QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));

    this->shader_program->bind();
    this->shader_program->setUniformValue("mat_model", mat_model); // 模型矩阵
    this->shader_program->setUniformValue("mat_view", mat_view);   // 摄像机矩阵

    /* 重新绑定 VAO */
    glBindVertexArray(VAO);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
