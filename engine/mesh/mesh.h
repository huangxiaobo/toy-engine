#ifndef __MESH_H__
#define __MESH_H__

#include <glm/glm.hpp>
#include <vector>
#include <string>
// OpenGL类型前向声明
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
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
    
    void SetTexture(unsigned int textureID) { m_textureID = textureID; }
    unsigned int GetTexture() const { return m_textureID; }

    // 设置法线贴图纹理ID（用于切线空间的法线映射）
    void SetNormalMap(unsigned int normalMapID) { m_normalMapID = normalMapID; }
    unsigned int GetNormalMap() const { return m_normalMapID; }

    virtual void Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::vec3 &camera, const std::vector<Light *> &lights);

    /*
     * 设置阴影深度 Pass 的全局状态（由 Renderer 在每帧切到阴影 FBO 时调用）
     *
     * 当 active 为 true 时，所有 Mesh::Draw 会改走"只写深度的阴影 Pass"：
     * 用 gDepthTech 和 lightSpace 变换，绘制同样几何但不做光照计算。
     * active 为 false 时恢复正常的光照绘制。
     */
    static void SetShadowDepthState(Technique *tech, const glm::mat4 &lightSpace, bool active);

    /*
     * 设置本帧是否已有可用的阴影深度贴图
     *
     * 主渲染 Pass 只有在存在有效阴影贴图时才上传 lightSpace/绑定 shadowMap，
     * 避免在阴影被禁用时采样到未绑定的纹理导致画面整体变暗。
     */
    static void SetShadowMapAvailable(bool available);

    static vector<Mesh *> CreatePlaneMesh();
    static vector<Mesh *> CreateGroundMesh();
    static vector<Mesh *> CreateTexturedGroundMesh(float size, int repeatCount);
    static vector<Mesh *> CreatePointMesh(glm::vec3 pos, glm::vec3 color);

    Mesh *Clone();
    void UpdateVertexBuffer();

private:
    void SetUpMesh();



public:
    vector<Vertex>& GetVertices();

public:
    string name;
    vector<Vertex> vertices;
    vector<GLuint> indices;

    GLuint VAO; // 创建 VAO 顶点数组对象
    GLuint VBO; // 创建 VBO 顶点缓冲对象
    GLuint EBO; // 创建 EBO 元素缓冲对象

    GLuint DrawMode; // 绘制模式

    Technique *m_effect;  // 裸指针，只引用不拥有，由外部管理生命周期
    
    unsigned int m_textureID; // 纹理ID
    
    unsigned int m_normalMapID; // 法线贴图纹理ID（切线空间法线映射用），0 表示未使用
};

#endif