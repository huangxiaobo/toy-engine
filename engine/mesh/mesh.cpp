#include "mesh.h"
#include <glad/gl.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>

Mesh::Mesh() : DrawMode(GL_TRIANGLES)
{
}

Mesh::Mesh(const vector<Vertex> &vertices, const vector<GLuint> &indices)
{
    DrawMode = GL_TRIANGLES;
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
    glEnableVertexAttribArray(Vertex::NormalLocation);                                                                      // 开始 VAO 管理的第二个属性值
    glVertexAttribPointer(Vertex::NormalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal)); // 手动传入第几个属性

    // Vertex TexCoords
    glEnableVertexAttribArray(Vertex::TexCoordsLocation);                                                                         // 开始 VAO 管理的第三个属性值
    glVertexAttribPointer(Vertex::TexCoordsLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords)); // 手动传入第几个属性

    // Vertex TexCoords
    glEnableVertexAttribArray(Vertex::TangentLocation);                                                                       // 开始 VAO 管理的第三个属性值
    glVertexAttribPointer(Vertex::TangentLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Tangent)); // 手动传入第几个属性

    // Vertex TexCoords
    glEnableVertexAttribArray(Vertex::BitangentLocation);                                                                         // 开始 VAO 管理的第三个属性值
    glVertexAttribPointer(Vertex::BitangentLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Bitangent)); // 手动传入第几个属性

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

void Mesh::SetDrawMode(GLuint mode)
{
    DrawMode = mode;
}

void Mesh::Draw(long long elapsed)
{
    glLineWidth(5.0f);
    /* 重新绑定 VAO */
    glBindVertexArray(VAO);
    // 绘制模式(DrawMode): GL_TRIANGLES, GL_LINES, GL_POINTS
    glDrawElements(DrawMode, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, (void *)0);
    glBindVertexArray(0);
}

vector<Mesh *> Mesh::CreatePlaneMesh()
{
    vector<Mesh *> meshes;

    vector<Vertex> vertices = {
        {
            // top right
            glm::vec3(0.5f, 0.5f, 0.0f), // Position
            glm::vec3(1.0f, 0.0f, 0.0f), // Color
            glm::vec3(0.0f, 0.0f, 0.0f), // Normal
            glm::vec2(1.0f, 1.0f),       // texture coords

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
            glm::vec3(-0.5f, -0.5f, 0.0f), // Position
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
    vector<unsigned int> indices = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    Mesh *mesh = new Mesh(vertices, indices);
    mesh->SetDrawMode(GL_TRIANGLES);

    meshes.push_back(mesh);
    return meshes;
}

vector<Mesh *> Mesh::CreateGroundMesh()
{
    vector<Mesh *> meshes;

    Mesh *m1 = new Mesh();
    m1->SetDrawMode(GL_LINES);

    int gridNum = 100;

    // draw grid 右手坐标系，逆时针方向排列
    //   + -------------------------------> x
    //   |
    //   |     (xi+0, zi+0)    (xi+1, zi+0)
    //   |     (xi+0, zi+1)    (xi+1, zi+1)
    //   |
    //   v
    //   z
    GLuint i = 0;
    for (int xi = -gridNum / 2; xi <= gridNum / 2; xi += 1)
    {
        for (int zi = -gridNum / 2; zi <= gridNum / 2; zi += 1)
        {
            m1->vertices.push_back(Vertex{
                glm::vec3{xi + 0, 0.0f, zi + 0},
                glm::vec3{0.5, 0.5, 0.5},
                glm::vec3{0.0, 1.0, 0.0},
                glm::vec2{0.0, 0.0},
            });

            m1->vertices.push_back(Vertex{
                glm::vec3{xi + 1, 0.0f, zi + 0},
                glm::vec3{0.5, 0.5, 0.5},
                glm::vec3{0.0, 1.0, 0.0},
                glm::vec2{0.0, 0.0},
            });

            m1->vertices.push_back(Vertex{
                glm::vec3{xi + 1, 0.0f, zi + 1},
                glm::vec3{0.5, 0.5, 0.5},
                glm::vec3{0.0, 1.0, 0.0},
                glm::vec2{0.0, 0.0},
            });

            m1->vertices.push_back(Vertex{
                glm::vec3{xi + 0, 0.0f, zi + 1},
                glm::vec3{0.5, 0.5, 0.5},
                glm::vec3{0.0, 1.0, 0.0},
                glm::vec2{0.0, 0.0},
            });
            m1->indices.push_back(i + 0);
            m1->indices.push_back(i + 1);
            m1->indices.push_back(i + 1);
            m1->indices.push_back(i + 2);
            m1->indices.push_back(i + 2);
            m1->indices.push_back(i + 3);
            m1->indices.push_back(i + 3);
            m1->indices.push_back(i + 0);
            i += 4;
        }
    }

    meshes.push_back(m1);

    for (auto mesh : meshes)
    {
        mesh->SetUpMesh();
    }
    return meshes;
}

vector<Mesh *> Mesh::CreatePointMesh(glm::vec3 pos, glm::vec3 color)
{
    vector<Mesh *> meshes;

    vector<Vertex> vertices = {
        {
            glm::vec3(pos.x, pos.y, pos.z),       // Position
            glm::vec3(color.x, color.y, color.z), // Color
            glm::vec3(0.0f, 0.0f, 0.0f),          // Normal
            glm::vec2(1.0f, 1.0f),                // texture co
        },
    };
    vector<unsigned int> indices = {
        0,
    };

    Mesh *m = new Mesh(vertices, indices);
    m->SetDrawMode(GL_POINTS);

    meshes.push_back(m);

    return meshes;
}

vector<glm::vec3> drawConeArrow(float length, float arrowLength, float arrowRadius, int segments = 20)
{
    // 圆锥底部的圆
    std::vector<float> circleVertices;
    for (int i = 0; i <= segments; ++i)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = arrowRadius * cos(angle);
        float y = arrowRadius * sin(angle);
        circleVertices.push_back(x);
        circleVertices.push_back(y);
        circleVertices.push_back(length);
    }
    vector<glm::vec3> points;
    points.push_back(glm::vec3(0.0f, 0.0f, length + arrowLength));
    for (size_t i = 0; i < circleVertices.size(); i += 3)
    {
        points.push_back(glm::vec3(circleVertices[i], circleVertices[i + 1], circleVertices[i + 2]));
    };
    return points;
}

vector<glm::vec3> createCirclePoints(float radius, int segments = 20, glm::vec3 center = glm::vec3(0.0f), glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f))
{
    // 圆锥底部的圆
    std::vector<glm::vec3> circleVertices;
    for (int i = 0; i <= segments; ++i)
    {
        glm::vec3 vertic;
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);

        if (up.x != 0.0f)
        {
            vertic = glm::vec3(0.0f, x, y);
        }
        else if (up.y != 0.0f)
        {
            vertic = glm::vec3(x, 0.0f, y);
        }
        else if (up.z != 0.0f)
        {
            vertic = glm::vec3(x, y, 0.0f);
        }

        circleVertices.push_back(vertic + center);
    }
    return circleVertices;
}

vector<Mesh *> Mesh::CreateAxisMesh()
{
    vector<Mesh *> meshes;

    auto axislength = 10.0f;
    auto axisRadius = 0.05f;
    auto arrowRadius = 0.2f;
    auto arrowLength = 0.7f;
    vector<glm::vec3> arrow_circle_points;
    vector<glm::vec3> circle_start_points;
    vector<glm::vec3> circle_end_points;

    // 绘制x轴
    circle_start_points = createCirclePoints(axisRadius, 20, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    circle_end_points = createCirclePoints(axisRadius, 20, glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    Mesh *m_x = new Mesh();
    for (size_t i = 0; i < circle_start_points.size(); i += 1)
    {
        m_x->vertices.push_back({circle_start_points[i],
                                 glm::vec3(1.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f),
                                 glm::vec2(0.0f)});
        m_x->vertices.push_back({circle_end_points[i],
                                 glm::vec3(1.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f),
                                 glm::vec2(0.0f)});
    }
    for (int i = 0; i < m_x->vertices.size() - 2; i += 2)
    {
        m_x->indices.push_back(i);
        m_x->indices.push_back(i + 1);
        m_x->indices.push_back(i + 2);
        m_x->indices.push_back(i + 1);
        m_x->indices.push_back(i + 2);
        m_x->indices.push_back(i + 3);
    }
    meshes.push_back(m_x);

    // 绘制x箭头
    Mesh *m_x_arrow = new Mesh();
    // 圆锥底部的圆
    arrow_circle_points = createCirclePoints(arrowRadius, 20, glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    m_x_arrow->vertices.push_back({glm::vec3(10.0f + arrowLength, 0.0f, 0.0f),
                                   glm::vec3(1.0f, 0.0f, 0.0f),
                                   glm::vec3(0.0f),
                                   glm::vec2(0.0f)});
    for (size_t i = 0; i < arrow_circle_points.size(); i += 1)
    {
        m_x_arrow->vertices.push_back({arrow_circle_points[i],
                                       glm::vec3(1.0f, 0.0f, 0.0f),
                                       glm::vec3(0.0f),
                                       glm::vec2(0.0f)});
    }

    for (size_t i = 2; i < m_x_arrow->vertices.size(); i += 1)
    {
        m_x_arrow->indices.push_back(0);
        m_x_arrow->indices.push_back(i - 1);
        m_x_arrow->indices.push_back(i);
    }
    meshes.push_back(m_x_arrow);

    // 绘制y轴
    circle_start_points = createCirclePoints(axisRadius, 20, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    circle_end_points = createCirclePoints(axisRadius, 20, glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    Mesh *m_y = new Mesh();
    for (size_t i = 0; i < circle_start_points.size(); i += 1)
    {
        m_y->vertices.push_back({circle_start_points[i],
                                 glm::vec3(0.0f, 1.0f, 0.0f),
                                 glm::vec3(0.0f),
                                 glm::vec2(0.0f)});
        m_y->vertices.push_back({circle_end_points[i],
                                 glm::vec3(0.0f, 1.0f, 0.0f),
                                 glm::vec3(0.0f),
                                 glm::vec2(0.0f)});
    }
    for (int i = 0; i < m_y->vertices.size() - 2; i += 2)
    {
        m_y->indices.push_back(i);
        m_y->indices.push_back(i + 1);
        m_y->indices.push_back(i + 2);
        m_y->indices.push_back(i + 1);
        m_y->indices.push_back(i + 2);
        m_y->indices.push_back(i + 3);
    }
    meshes.push_back(m_y);

    // 绘制y箭头
    Mesh *m_y_arrow = new Mesh();
    // 圆锥底部的圆
    arrow_circle_points = createCirclePoints(arrowRadius, 20, glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    m_y_arrow->vertices.push_back({glm::vec3(0.0f, 10.0f + arrowLength, 0.0f),
                                   glm::vec3(0.0f, 1.0f, 0.0f),
                                   glm::vec3(0.0f),
                                   glm::vec2(0.0f)});
    for (size_t i = 0; i < arrow_circle_points.size(); i += 1)
    {
        m_y_arrow->vertices.push_back({arrow_circle_points[i],
                                       glm::vec3(0.0f, 1.0f, 0.0f),
                                       glm::vec3(0.0f),
                                       glm::vec2(0.0f)});
    }

    for (size_t i = 2; i < m_y_arrow->vertices.size(); i += 1)
    {
        m_y_arrow->indices.push_back(0);
        m_y_arrow->indices.push_back(i - 1);
        m_y_arrow->indices.push_back(i);
    }
    meshes.push_back(m_y_arrow);

    // 绘制z轴
    circle_start_points = createCirclePoints(axisRadius, 20, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    circle_end_points = createCirclePoints(axisRadius, 20, glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    Mesh *m_z = new Mesh();
    for (size_t i = 0; i < circle_start_points.size(); i += 1)
    {
        m_z->vertices.push_back({circle_start_points[i],
                                 glm::vec3(0.0f, 0.0f, 1.0f),
                                 glm::vec3(0.0f),
                                 glm::vec2(0.0f)});
        m_z->vertices.push_back({circle_end_points[i],
                                 glm::vec3(0.0f, 0.0f, 1.0f),
                                 glm::vec3(0.0f),
                                 glm::vec2(0.0f)});
    }
    for (int i = 0; i < m_z->vertices.size() - 2; i += 2)
    {
        m_z->indices.push_back(i);
        m_z->indices.push_back(i + 1);
        m_z->indices.push_back(i + 2);
        m_z->indices.push_back(i + 1);
        m_z->indices.push_back(i + 2);
        m_z->indices.push_back(i + 3);
    }
    meshes.push_back(m_z);

    // 绘制z箭头
    Mesh *m_z_arrow = new Mesh();
    // 圆锥底部的圆
    arrow_circle_points = createCirclePoints(arrowRadius, 20, glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    m_z_arrow->vertices.push_back({glm::vec3(0.0f, 0.0f, 10.0f + arrowLength),
                                   glm::vec3(0.0f, 0.0f, 1.0f),
                                   glm::vec3(0.0f),
                                   glm::vec2(0.0f)});
    for (size_t i = 0; i < arrow_circle_points.size(); i += 1)
    {
        m_z_arrow->vertices.push_back({arrow_circle_points[i],
                                       glm::vec3(0.0f, 0.0f, 1.0f),
                                       glm::vec3(0.0f),
                                       glm::vec2(0.0f)});
    }

    for (size_t i = 2; i < m_z_arrow->vertices.size(); i += 1)
    {
        m_z_arrow->indices.push_back(0);
        m_z_arrow->indices.push_back(i - 1);
        m_z_arrow->indices.push_back(i);
    }
    meshes.push_back(m_z_arrow);

    // // 绘制箭头
    // Mesh *m2 = new Mesh();
    // // 圆锥底部的圆
    // auto arrowRadius = 0.2f;
    // std::vector<glm::vec3> circleVertices = drawConeArrow(10.0f, 1.0f, arrowRadius);
    // for (size_t i = 0; i < circleVertices.size(); i += 1)
    // {
    //     m2->vertices.push_back({circleVertices[i],
    //                             glm::vec3(1.0f),
    //                             glm::vec3(0.0f),
    //                             glm::vec2(0.0f)});
    // }

    // for (size_t i = 2; i < m2->vertices.size(); i += 1)
    // {
    //     m2->indices.push_back(0);
    //     m2->indices.push_back(i - 1);
    //     m2->indices.push_back(i);
    // }
    // meshes.push_back(m2);

    // Mesh *m3 = new Mesh();
    // m3->vertices = vector<Vertex>(m2->vertices);
    // m3->indices = vector<unsigned int>(m2->indices);

    // // 定义旋转角度（45度）和旋转轴（Z轴）
    // float angle = glm::radians(-90.0f); // 将角度转换为弧度
    // glm::vec3 axis(1.0f, 0.0f, 0.0f);   // 绕Z轴旋转

    // // 创建旋转矩阵
    // glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, axis);

    // for (auto &v : m3->vertices)
    // {
    //     auto point = rotationMatrix * glm::vec4(v.Position, 0.0);
    //     v.Position = glm::vec3(point);
    // }
    // meshes.push_back(m3);

    // Mesh *m4 = new Mesh();
    // m4->vertices = vector<Vertex>(m2->vertices);
    // m4->indices = vector<unsigned int>(m2->indices);

    // // 定义旋转角度（45度）和旋转轴（Z轴）
    // float angleY = glm::radians(90.0f); // 将角度转换为弧度
    // glm::vec3 axisY(0.0f, 1.0f, 0.0f);  // 绕Z轴旋转

    // // 创建旋转矩阵
    // glm::mat4 rotationMatrixY = glm::rotate(glm::mat4(1.0f), angleY, axisY);

    // for (auto &v : m4->vertices)
    // {
    //     auto point = rotationMatrixY * glm::vec4(v.Position, 0.0);
    //     v.Position = glm::vec3(point);
    // }
    // meshes.push_back(m4);

    for (auto mesh : meshes)
    {
        mesh->SetUpMesh();
    }

    return meshes;
}

Mesh *Mesh::Clone()
{

    Mesh *m = new Mesh();
    m->vertices = vector<Vertex>(this->vertices);
    m->indices = vector<unsigned int>(this->indices);
    m->name = this->name;
    m->SetUpMesh();
    return m;
}

// 辅助函数：归一化向量
glm::vec3 normalize(const glm::vec3 &v)
{
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return glm::vec3(v.x / length, v.y / length, v.z / length);
}

// 辅助函数：细分三角形
void subdivide(vector<Vertex> &vertices, vector<unsigned int> &indices)
{
    vector<unsigned int> newIndices;
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];

        glm::vec3 v0 = vertices[i0].Position;
        glm::vec3 v1 = vertices[i1].Position;
        glm::vec3 v2 = vertices[i2].Position;

        glm::vec3 v01 = normalize(v0 + v1);
        glm::vec3 v12 = normalize(v1 + v2);
        glm::vec3 v20 = normalize(v2 + v0);

        int i01 = vertices.size();
        vertices.push_back({v01, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)});
        int i12 = vertices.size();
        vertices.push_back({v12, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)});
        int i20 = vertices.size();
        vertices.push_back({v20, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)});

        newIndices.push_back(i0);
        newIndices.push_back(i01);
        newIndices.push_back(i20);

        newIndices.push_back(i01);
        newIndices.push_back(i1);
        newIndices.push_back(i12);

        newIndices.push_back(i12);
        newIndices.push_back(i2);
        newIndices.push_back(i20);

        newIndices.push_back(i01);
        newIndices.push_back(i12);
        newIndices.push_back(i20);
    }
    indices = newIndices;
}

vector<Mesh *> Mesh::CreateIcosphereMesh(int subdivisions)
{
    vector<Mesh *> meshes;

    // 二十面体的顶点和索引数据
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    vector<Vertex> vertices = {
        {{-1, t, 0}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{1, t, 0}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{-1, -t, 0}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{1, -t, 0}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{0, -1, t}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{0, 1, t}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{0, -1, -t}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{0, 1, -t}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{t, 0, -1}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{t, 0, 1}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{-t, 0, -1}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)},
        {{-t, 0, 1}, glm::vec3(1.0f), glm::vec3(0.0f), glm::vec2(0.0f)}};
    for (auto &v : vertices)
    {
        v.Position = normalize(v.Position);
    }

    vector<unsigned int> indices = {
        0, 11, 5,
        0, 5, 1,
        0, 1, 7,
        0, 7, 10,
        0, 10, 11,
        1, 5, 9,
        5, 11, 4,
        11, 10, 2,
        10, 7, 6,
        7, 1, 8,
        3, 9, 4,
        3, 4, 2,
        3, 2, 6,
        3, 6, 8,
        3, 8, 9,
        4, 9, 5,
        2, 4, 11,
        6, 2, 10,
        8, 6, 7,
        9, 8, 1};

    // 细分三角形
    for (int i = 0; i < subdivisions; ++i)
    {
        subdivide(vertices, indices);
    }

    // 创建 Mesh 对象
    Mesh *m = new Mesh(vertices, indices);
    m->SetDrawMode(GL_TRIANGLES);

    meshes.push_back(m);

    return meshes;
}
