#ifndef __MESH_H__
#define __MESH_H__

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <glad/gl.h>
#include <iostream>

using namespace std;

class Light;
class Technique;

class Vertex
{
public:
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;

    alignas(16) static constexpr int PositionLocation = 0;
    alignas(16) static constexpr int ColorLocation = 1;
    alignas(16) static constexpr int NormalLocation = 2;
    alignas(16) static constexpr int TexCoordsLocation = 3;
    alignas(16) static constexpr int TangentLocation = 4;
    alignas(16) static constexpr int BitangentLocation = 5;

public:
    void Debug()
    {
        std::cout << "Vertex:" << std::endl;
        std::cout << "  Position: " << Position.x << ", " << Position.y << ", " << Position.z << std::endl;
        std::cout << "  Color: " << Color.x << ", " << Color.y << ", " << Color.z << std::endl;
        std::cout << "  Normal: " << Normal.x << ", " << Normal.y << ", " << Normal.z << std::endl;
        std::cout << "  TexCoords: " << TexCoords.x << ", " << TexCoords.y << std::endl;
        std::cout << "  Tangent: " << Tangent.x << ", " << Tangent.y << ", " << Tangent.z << std::endl;
        std::cout << "  Bitangent: " << Bitangent.x << ", " << Bitangent.y << ", " << Bitangent.z << std::endl;
    }
};

class Mesh
{

public:
    Mesh();
    Mesh(const vector<Vertex> &vertices, const vector<GLuint> &indices);
    ~Mesh();

    void SetDrawMode(GLuint mode);

    void SetEffect(Technique *effect);
    Technique* GetEffect() const;

    virtual void Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::vec3 &camera, const std::vector<Light *> &lights);

    static vector<Mesh *> CreatePlaneMesh();
    static vector<Mesh *> CreateGroundMesh();
    static vector<Mesh *> CreatePointMesh(glm::vec3 pos, glm::vec3 color);
    static vector<Mesh *> CreateIcosphereMesh(int subdivisions = 0);
    static vector<Mesh *> CreateIcosphereMesh(int subdivisions, glm::vec3 center, glm::vec3 color);
    static vector<Mesh *> CreateAxisMesh();

    Mesh *Clone();

private:
    void SetUpMesh();

public:
    string name;
    vector<Vertex> vertices;
    vector<GLuint> indices;

    GLuint VAO; // 创建 VAO 顶点数组对象
    GLuint VBO; // 创建 VBO 顶点缓冲对象
    GLuint EBO; // 创建 EBO 元素缓冲对象

    GLuint DrawMode; // 绘制模式

    Technique *m_effect;
};

#endif