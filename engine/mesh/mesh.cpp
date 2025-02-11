#include "mesh.h"

#include <iostream>

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::SetUpMesh()
{
    // ===================== VAO | VBO =====================
    // VAO 和 VBO 对象赋予 ID
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 绑定 VAO、VBO 对象
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    /* 为当前绑定到 target 的缓冲区对象创建一个新的数据存储（在 GPU 上创建对应的存储区域，并将内存中的数据发送过去）
        如果 data 不是 NULL，则使用来自此指针的数据初始化数据存储
        void glBufferData(GLenum target,       // 需要在 GPU 上创建的目标
                          GLsizeipter size,    // 创建的显存大小
                          const GLvoid* data,  // 数据
                          GLenum usage)        // 创建在 GPU 上的哪一片区域（显存上的每个区域的性能是不一样的）https://registry.khronos.org/OpenGL-Refpages/es3.0/
    */
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.constData(), GL_STATIC_DRAW);

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

    // Vertex Positions
    // 开始 VAO 管理的第一个属性值
    glVertexAttribPointer(Vertex::PositionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0); // 手动传入第几个属性
    glEnableVertexAttribArray(Vertex::PositionLocation);

    // Vertex Color
    // 开始 VAO 管理的第二个属性值
    glVertexAttribPointer(Vertex::ColorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Color)); // 手动传入第几个属性
    glEnableVertexAttribArray(Vertex::ColorLocation);

    // Vertex Normal
    glEnableVertexAttribArray(2);                                                                      // 开始 VAO 管理的第二个属性值
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal)); // 手动传入第几个属性

    // Vertex TexCoords
    glEnableVertexAttribArray(3);                                                                         // 开始 VAO 管理的第三个属性值
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords)); // 手动传入第几个属性

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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.constData(), GL_STATIC_DRAW); // EBO/IBO 是储存顶点【索引】的

    // 解绑 VAO 和 VBO，注意先解绑 VAO再解绑EBO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // 注意 VAO 不参与管理 VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::Draw(long long elapsed)
{
    /* 重新绑定 VAO */
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void *)0);
    glBindVertexArray(0);
}

Mesh *Mesh::CreatePlaneMesh()
{
    Mesh *mesh = new Mesh();

    mesh->vertices = {
        {
            // top right
            QVector3D(0.5f, 0.5f, -0.5f), // Position
            QVector3D(1.0f, 0.0f, 0.0f),  // Color
            QVector3D(0.0f, 0.0f, 0.0f),  // Normal
            QVector2D(1.0f, 1.0f),        // texture coords

        },
        {
            // bottom right
            QVector3D(0.5f, -0.5f, 0.0f), // Position
            QVector3D(0.0f, 1.0f, 0.0f),  // Color
            QVector3D(0.0f, 0.0f, 0.0f),  // Normal
            QVector2D(1.0f, 1.0f),        // texture coords
        },
        {
            // bottom left
            QVector3D(-0.5f, -0.5f, 0.5f), // Position
            QVector3D(0.0f, 0.0f, 1.0f),   // Color
            QVector3D(0.0f, 0.0f, 0.0f),   // Normal
            QVector2D(1.0f, 1.0f),         // texture coords
        },
        {
            // top left
            QVector3D(-0.5f, 0.5f, 0.0f), // Position
            QVector3D(0.0f, 0.0f, 0.0f),  // Color
            QVector3D(0.0f, 0.0f, 0.0f),  // Normal
            QVector2D(1.0f, 1.0f),        // texture coords
        },
    };
    mesh->indices = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    mesh->SetUpMesh();

    return mesh;
}