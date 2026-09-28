#ifndef __MESH_H__
#define __MESH_H__

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
// GLuint/GLenum 等 GL 类型由 glad 提供，须先包含 glad

class Light;
class Material;
class Technique;
struct RenderContext;

class Vertex
{
public:
    glm::vec3 m_position;
    glm::vec3 m_color;
    glm::vec3 m_normal;
    glm::vec2 m_tex_coords;
    glm::vec3 m_tangent;
    glm::vec3 m_bitangent;

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
        std::cout << "  position: " << m_position.x << ", " << m_position.y << ", " << m_position.z << std::endl;
        std::cout << "  color: " << m_color.x << ", " << m_color.y << ", " << m_color.z << std::endl;
        std::cout << "  normal: " << m_normal.x << ", " << m_normal.y << ", " << m_normal.z << std::endl;
        std::cout << "  tex_coords: " << m_tex_coords.x << ", " << m_tex_coords.y << std::endl;
        std::cout << "  tangent: " << m_tangent.x << ", " << m_tangent.y << ", " << m_tangent.z << std::endl;
        std::cout << "  bitangent: " << m_bitangent.x << ", " << m_bitangent.y << ", " << m_bitangent.z << std::endl;
    }
};

class Mesh
{

public:
    Mesh();
    Mesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices);
    // 虚析构：Mesh 经基类指针/unique_ptr 持有派生对象，需可安全析构
    virtual ~Mesh();
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    void SetDrawMode(unsigned int mode);

    void SetEffect(Technique *effect);
    Technique* GetEffect() const;
    
    void SetTexture(unsigned int textureID) { m_texture_id = textureID; }
    unsigned int GetTexture() const { return m_texture_id; }

    // 设置法线贴图纹理ID（用于切线空间的法线映射）
    void SetNormalMap(unsigned int normalMapID) { m_normal_map_id = normalMapID; }
    unsigned int GetNormalMap() const { return m_normal_map_id; }

    // material 为本对象材质，由 Model 传入；渲染风格技术在多个模型间共享，
    // 因此材质只能在这里逐次上传，Technique 不得持有。传 nullptr 表示不覆盖。
    virtual void Draw(const RenderContext &ctx, const glm::mat4 &model, const Material *material);

    // 工厂方法返回 unique_ptr 容器，mesh 所有权随容器转移
    static std::vector<std::unique_ptr<Mesh>> CreatePlaneMesh();
    static std::vector<std::unique_ptr<Mesh>> CreateTexturedGroundMesh(float size, int repeatCount);

    void UpdateVertexBuffer();

private:
    void SetUpMesh();



public:
    std::vector<Vertex> &GetVertices();

public:
    std::string m_name;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;

    // GL 对象名与绘制状态在此统一给出默认值，两个构造函数共用同一套初值。
    // 不可依赖构造函数体内赋值：双参构造路径（模型/地形）曾漏掉 m_effect 初始化。
    unsigned int m_vao = 0; // 创建 VAO 顶点数组对象
    unsigned int m_vbo = 0; // 创建 vbo 顶点缓冲对象
    unsigned int m_ebo = 0; // 创建 ebo 元素缓冲对象

    unsigned int m_draw_mode = 0; // 绘制模式，由构造函数置为 GL_TRIANGLES

    Technique *m_effect = nullptr;  // 裸指针，只引用不拥有，由外部管理生命周期

    unsigned int m_texture_id = 0; // 纹理ID

    unsigned int m_normal_map_id = 0; // 法线贴图纹理ID（切线空间法线映射用），0 表示未使用
};

#endif