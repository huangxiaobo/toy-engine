#ifndef __MESH_H__
#define __MESH_H__

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <glad/gl.h>

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
};

class Mesh
{

public:
    Mesh();

    Mesh(const vector<Vertex>& vertices, const vector<GLuint>& indices);
    ~Mesh();
    void SetUpMesh();

    void Draw(long long elapsed);

    static  vector<Mesh*> CreatePlaneMesh();
    static vector<Mesh*> CreateGroundMesh();

public:
    string name;
    vector<Vertex> vertices;
    vector<GLuint> indices;

    GLuint VAO, VBO;
    GLuint EBO; // 创建 EBO 元素缓冲对象

    GLuint DrawMode;
};

#endif