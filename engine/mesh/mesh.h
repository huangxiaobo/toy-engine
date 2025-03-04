#ifndef __MESH_H__
#define __MESH_H__

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <glad/gl.h>
#include <iostream>

using namespace std;

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

    virtual void Draw(long long elapsed);

    static vector<Mesh *> CreatePlaneMesh();
    static vector<Mesh *> CreateGroundMesh();
    static vector<Mesh *> CreatePointMesh(float x, float y, float z);

    static vector<Mesh *> CreateAxisMesh();

private:
    void SetUpMesh();

public:
    string name;
    vector<Vertex> vertices;
    vector<GLuint> indices;

    GLuint VAO, VBO;
    GLuint EBO; // 创建 EBO 元素缓冲对象

    GLuint DrawMode;
};

#endif