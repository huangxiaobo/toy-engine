#include "mesh.h"
#include <glad/gl.h>
#include <iostream>

Mesh::Mesh()
{
}

Mesh::Mesh(const vector<Vertex>& vertices, const vector<GLuint>& indices)
{
    this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());
    this->indices.insert(this->indices.end(), indices.begin(), indices.end());
    this->SetUpMesh();
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
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW); // EBO/IBO 是储存顶点【索引】的

    // 解绑 VAO 和 VBO，注意先解绑 VAO再解绑EBO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // 注意 VAO 不参与管理 VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Mesh::Draw(long long elapsed)
{
    /* 重新绑定 VAO */
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, (void *)0);
    glBindVertexArray(0);
}

vector<Mesh *> Mesh::CreatePlaneMesh()
{
    vector<Mesh *> meshes;

    Mesh *mesh = new Mesh();

   
    mesh->vertices = {
        {
            // top right
            glm::vec3(0.5f, 0.5f, -0.5f), // Position
            glm::vec3(1.0f, 0.0f, 0.0f),  // Color
            glm::vec3(0.0f, 0.0f, 0.0f),  // Normal
            glm::vec2(1.0f, 1.0f),        // texture coords

        },
        {
            // bottom right
            glm::vec3(0.5f, -0.5f, 0.0f), // Position
            glm::vec3(0.0f, 1.0f, 0.0f),  // Color
            glm::vec3(0.0f, 0.0f, 0.0f),  // Normal
            glm::vec2(1.0f, 1.0f),        // texture coords
        },
        {
            // bottom left
            glm::vec3(-0.5f, -0.5f, 0.5f), // Position
            glm::vec3(0.0f, 0.0f, 1.0f),   // Color
            glm::vec3(0.0f, 0.0f, 0.0f),   // Normal
            glm::vec2(1.0f, 1.0f),         // texture coords
        },
        {
            // top left
            glm::vec3(-0.5f, 0.5f, 0.0f), // Position
            glm::vec3(0.0f, 0.0f, 0.0f),  // Color
            glm::vec3(0.0f, 0.0f, 0.0f),  // Normal
            glm::vec2(1.0f, 1.0f),        // texture coords
        },
    };
    mesh->indices = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    mesh->SetUpMesh();

    meshes.push_back(mesh);
    return meshes;
}

vector<Mesh *> Mesh::CreateGroundMesh()
{
    vector<Mesh *> meshes;

    Mesh *m1 = new Mesh();
    m1->DrawMode = GL_LINES;

    int gridNum = 10;
    float gridStrip = 1;
    float gridWidth = 1.0f * gridNum * gridStrip / 2;

    // draw grid
    GLuint i = 0;
    for (int k = -gridNum / 2; k <= gridNum / 2; k += 1)
    {
        if (k == 0)
        {
            continue;
        }
        m1->vertices.push_back(Vertex{
            glm::vec3{1.0f * k * gridStrip, 0.0f, +gridWidth},
            glm::vec3{0.0, 1.0, 0.0},
            glm::vec3{0.0, 1.0, 0.0},
            glm::vec2{0.0, 0.0},
        });
        m1->indices.push_back(i++);

        m1->vertices.push_back(Vertex{
            glm::vec3{1.0f * k * gridStrip, 0.0f, -gridWidth},
            glm::vec3{0.0, 1.0, 0.0},
            glm::vec3{0.0, 1.0, 0.0},
            glm::vec2{0.0, 0.0},
        });
        m1->indices.push_back(i++);

        m1->vertices.push_back(Vertex{
            glm::vec3{+gridWidth, 0.0f, 1.0f * k * gridStrip},
            glm::vec3{0.0, 1.0, 0.0},
            glm::vec3{0.0, 1.0, 0.0},
            glm::vec2{0.0, 0.0},
        });
        m1->indices.push_back(i++);

        m1->vertices.push_back(Vertex{
            glm::vec3{-gridWidth, 0.0f, 1.0f * k * gridStrip},
            glm::vec3{0.0, 1.0, 0.0},
            glm::vec3{0.0, 1.0, 0.0},
            glm::vec2{0.0, 0.0},
        });
        m1->indices.push_back(i++);
    }

    meshes.push_back(m1);

    Mesh *m = new Mesh();
    m->DrawMode = GL_LINES;

    i = 0;

    m->vertices.push_back(Vertex{
        glm::vec3{0.0f, 0.0f, -gridWidth},
        glm::vec3{0.0, 1.0, 0.0},
        glm::vec3{0.0, 1.0, 0.0},
        glm::vec2{0.0, 0.0},
    });
    m->indices.push_back(i++);

    m->vertices.push_back(Vertex{
        glm::vec3{0.0f, 0.0f, +gridWidth},
        glm::vec3{0.0, 1.0, 0.0},
        glm::vec3{0.0, 1.0, 0.0},
        glm::vec2{0.0, 0.0},
    });
    m->indices.push_back(i++);

    m->vertices.push_back(Vertex{
        glm::vec3{+gridWidth, 0.0f, 0.0f},
        glm::vec3{0.0, 1.0, 0.0},
        glm::vec3{0.0, 1.0, 0.0},
        glm::vec2{0.0, 0.0},
    });
    m->indices.push_back(i++);

    m->vertices.push_back(Vertex{
        glm::vec3{-gridWidth, 0.0f, 0.0f},
        glm::vec3{0.0, 1.0, 0.0},
        glm::vec3{0.0, 1.0, 0.0},
        glm::vec2{0.0, 0.0},
    });
    m->indices.push_back(i++);

    meshes.push_back(m);

    for (auto mesh: meshes)
    {
        mesh->SetUpMesh();
    }
    return meshes;
}
