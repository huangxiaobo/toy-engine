#ifndef __MESH_H__
#define __MESH_H__

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
// GLuint/GLenum 等 GL 类型由 glad 提供，须先包含 glad

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

    static constexpr int PositionLocation = 0;
    static constexpr int ColorLocation = 1;
    static constexpr int NormalLocation = 2;
    static constexpr int TexCoordsLocation = 3;
    static constexpr int TangentLocation = 4;
    static constexpr int BitangentLocation = 5;

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
    Mesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices);
    // 虚析构：Mesh 经基类指针/unique_ptr 持有派生对象，需可安全析构
    virtual ~Mesh();

    void SetDrawMode(unsigned int mode);

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

    // 工厂方法返回 unique_ptr 容器，mesh 所有权随容器转移
    static std::vector<std::unique_ptr<Mesh>> CreatePlaneMesh();
    static std::vector<std::unique_ptr<Mesh>> CreateTexturedGroundMesh(float size, int repeatCount);

    void UpdateVertexBuffer();

private:
    void SetUpMesh();



public:
    std::vector<Vertex> &GetVertices();

public:
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO; // 创建 VAO 顶点数组对象
    unsigned int VBO; // 创建 VBO 顶点缓冲对象
    unsigned int EBO; // 创建 EBO 元素缓冲对象

    unsigned int DrawMode; // 绘制模式

    Technique *m_effect;  // 裸指针，只引用不拥有，由外部管理生命周期
    
    unsigned int m_textureID; // 纹理ID
    
    unsigned int m_normalMapID; // 法线贴图纹理ID（切线空间法线映射用），0 表示未使用
};

#endif