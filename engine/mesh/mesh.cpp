// glad 必须第一个包含：GLuint/GLenum 等类型由此提供
#include <glad/gl.h>
#include "mesh.h"
#include <iostream>

using namespace std;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdlib>

#include "../utils/utils.h"
#include "../technique/technique.h"
#include "../render_context.h"

// 该文件不再持有阴影文件级静态量：阴影深度 Pass 状态由 Renderer 每帧组装进
// RenderContext.shadow（见 render_context.h），Mesh::Draw 直接从 ctx 读取，
// 与地形链路共用同一条状态通道，避免两处全局状态不同步。

Mesh::Mesh() : DrawMode(GL_TRIANGLES) {
    VAO = 0;
    VBO = 0;
    EBO = 0;
    m_textureID = 0;
    m_normalMapID = 0;
}

Mesh::Mesh(const vector<Vertex> &vertices, const vector<unsigned int> &indices) {
    DrawMode = GL_TRIANGLES;
    this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());
    this->indices.insert(this->indices.end(), indices.begin(), indices.end());
    this->SetUpMesh();
}

Mesh::~Mesh() {
    // m_effect 是裸指针，只引用不拥有，不需要释放
}

void Mesh::SetUpMesh() {
    // ===================== VAO | VBO =====================
    // 创建并绑定VAO，VAO是一种容器对象，它存储了多个VBO以及与这些VBO相关的顶点属性指针设置（即glVertexAttribPointer的调用）
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // 创建并绑定VBO，VBO是一个GPU上的内存缓冲区，用来存储顶点属性的数据，如位置、颜色、纹理坐标、法线等信息
    glGenBuffers(1, &VBO);
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
    glVertexAttribPointer(Vertex::PositionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0); // 手动传入第几个属性
    glEnableVertexAttribArray(Vertex::PositionLocation);

    // Vertex Color
    // 开始 VAO 管理的第二个属性值
    glVertexAttribPointer(Vertex::ColorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, Color)); // 手动传入第几个属性
    glEnableVertexAttribArray(Vertex::ColorLocation);

    // Vertex Normal
    glEnableVertexAttribArray(Vertex::NormalLocation); // 开始 VAO 管理的第二个属性值
    glVertexAttribPointer(Vertex::NormalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, Normal)); // 手动传入第几个属性

    // Vertex TexCoords
    glEnableVertexAttribArray(Vertex::TexCoordsLocation); // 开始 VAO 管理的第三个属性值
    glVertexAttribPointer(Vertex::TexCoordsLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, TexCoords)); // 手动传入第几个属性

    // Vertex TexCoords
    glEnableVertexAttribArray(Vertex::TangentLocation); // 开始 VAO 管理的第三个属性值
    glVertexAttribPointer(Vertex::TangentLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, Tangent)); // 手动传入第几个属性

    // Vertex TexCoords
    glEnableVertexAttribArray(Vertex::BitangentLocation); // 开始 VAO 管理的第三个属性值
    glVertexAttribPointer(Vertex::BitangentLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, Bitangent)); // 手动传入第几个属性

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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    // EBO/IBO 是储存顶点【索引】的

    // 解绑 VAO 和 VBO，注意先解绑 VAO再解绑EBO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // 注意 VAO 不参与管理 VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::UpdateVertexBuffer() {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
}

vector<Vertex> &Mesh::GetVertices() {
    return this->vertices;
}


void Mesh::SetDrawMode(unsigned int mode) {
    DrawMode = mode;
}

void Mesh::SetEffect(Technique *effect) {
    m_effect = effect;
}

Technique *Mesh::GetEffect() const {
    return m_effect;
}

void Mesh::Draw(const RenderContext &ctx, const glm::mat4 &model) {
    // 阴影深度 Pass：Renderer 组装 ctx.shadow 后，本函数以光源视角只写深度绘制阴影贴图。
    // 此时不做光照计算，仅用 gDepthMVP(=lightSpace*model) 变换同一份几何，
    // 保证深度贴图几何与主 Pass 完全一致、阴影不错位。
    if (ctx.shadow.passActive) {
        if (ctx.shadow.depthTech == nullptr) {
            return;
        }
        ctx.shadow.depthTech->Enable();
        ctx.shadow.depthTech->SetUniform("gDepthMVP", ctx.shadow.lightSpace * model);
        /* 重新绑定 VAO */
        glBindVertexArray(VAO);
        glDrawElements(DrawMode, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, (void *) 0);
        glBindVertexArray(0);
        return;
    }

    this->m_effect->Enable();
    this->m_effect->SetProjectionMatrix(ctx.projection);
    this->m_effect->SetViewMatrix(ctx.view);
    this->m_effect->SetModelMatrix(model);
    // this->m_effect->SetWVPMatrix(mvp);
    this->m_effect->SetCamera(ctx.camera);

    this->m_effect->SetLights(ctx.lights);

    // 阴影贴图相关 uniform：
    //   - gUseShadow 始终设置：阴影可用时为1（着色器据此启用阴影计算），否则为0
    //   - 仅当阴影贴图存在时才上传 lightSpace / 绑定 shadowMap，
    //     避免在阴影被禁用时让着色器采样未绑定纹理导致画面整体变暗
    this->m_effect->SetUniform("gUseShadow", ctx.shadow.ready ? 1 : 0);
    if (ctx.shadow.ready) {
        this->m_effect->SetShadowMap(2);
        this->m_effect->SetLightSpaceMatrix(ctx.shadow.lightSpace);
        // bias 缩放系数跟随面板滑块（1.0 = 原始公式），正交范围扩大/缩小时需微调抗痤疮
        this->m_effect->SetUniform("gShadowBiasScale", ctx.shadow.biasScale);
    }

    // 【调试用】ShadowCalculation 中间结果输出开关：
    //   通过环境变量 TOY_DEBUG_SHADOW 控制（0~5，见 bunny/shader.frag 的 gDebugShadow 说明）。
    //   0=正常阴影逻辑；1~5 分别把 projCoords.xy / currentDepth / closestDepth / bias / shadow
    //   输出到颜色以便肉眼定位哪一步计算不符合预期。启动时读取一次即可。
    //   没有该 uniform 的着色器（纯色光照模型）location 为 -1，SetUniform 内部会安全忽略。
    static int s_debugShadow = -1;
    if (s_debugShadow < 0) {
        const char *e = getenv("TOY_DEBUG_SHADOW");
        s_debugShadow = e ? atoi(e) : 0;
    }
    if (s_debugShadow > 0) {
        this->m_effect->SetUniform("gDebugShadow", s_debugShadow);
    }

    // 绑定纹理：漫反射贴图用第0纹理单元（gTexture），法线贴图用第1纹理单元（gNormalMap）
    // gHasTexture / gHasNormalMap 标志供着色器判断是否采样贴图：
    // 无贴图时（m_textureID == 0）关闭采样，避免采样到残留/脏纹理单元导致的错误着色
    this->m_effect->SetUniform("gHasTexture", m_textureID != 0 ? 1 : 0);
    if (m_textureID != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        // 通知着色器：漫反射采样器绑定到纹理单元0
        this->m_effect->SetUniform("gTexture", 0);
    }

    this->m_effect->SetUniform("gHasNormalMap", m_normalMapID != 0 ? 1 : 0);
    if (m_normalMapID != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_normalMapID);
        // 通知着色器：法线贴图采样器绑定到纹理单元1
        this->m_effect->SetUniform("gNormalMap", 1);
    }

    /* 重新绑定 VAO */
    glBindVertexArray(VAO);
    // 绘制模式(DrawMode): GL_TRIANGLES, GL_LINES, GL_POINTS
    glDrawElements(DrawMode, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, (void *) 0);
    glBindVertexArray(0);
}

std::vector<std::unique_ptr<Mesh>> Mesh::CreatePlaneMesh() {
    std::vector<std::unique_ptr<Mesh>> meshes;

    vector<Vertex> vertices = {
        {
            // top right
            glm::vec3(0.5f, 0.5f, 0.0f), // Position
            glm::vec3(1.0f, 0.0f, 0.0f), // Color
            glm::vec3(0.0f, 0.0f, 0.0f), // Normal
            glm::vec2(1.0f, 1.0f), // texture coords

        },
        {
            // bottom right
            glm::vec3(0.5f, -0.5f, 0.0f), // Position
            glm::vec3(0.0f, 1.0f, 0.0f), // Color
            glm::vec3(0.0f, 0.0f, 0.0f), // Normal
            glm::vec2(1.0f, 1.0f), // texture coords
        },
        {
            // bottom left
            glm::vec3(-0.5f, -0.5f, 0.0f), // Position
            glm::vec3(0.0f, 0.0f, 1.0f), // Color
            glm::vec3(0.0f, 0.0f, 0.0f), // Normal
            glm::vec2(1.0f, 1.0f), // texture coords
        },
        {
            // top left
            glm::vec3(-0.5f, 0.5f, 0.0f), // Position
            glm::vec3(0.0f, 0.0f, 0.0f), // Color
            glm::vec3(0.0f, 0.0f, 0.0f), // Normal
            glm::vec2(1.0f, 1.0f), // texture coords
        },
    };
    vector<unsigned int> indices = {
        0, 1, 3, // first triangle
        1, 2, 3 // second triangle
    };

    auto mesh = std::make_unique<Mesh>(vertices, indices);
    mesh->SetDrawMode(GL_TRIANGLES);

    meshes.push_back(std::move(mesh));
    return meshes;
}

/*
 * 创建带 UV 纹理坐标的平面网格（用于地形/地面）
 *
 * 参数：
 *   size        - 平面边长（世界单位），平面以原点为中心，位于 XZ 平面（y=0）
 *   repeatCount - 纹理重复次数：UV 在 [0, repeatCount] 区间取值，
 *                 使棋盘格等纹理在整块地面上重复铺贴而非拉伸
 *
 * 法线统一朝上 (0,1,0)，颜色白色以便纹理显示原色。
 */
std::vector<std::unique_ptr<Mesh>> Mesh::CreateTexturedGroundMesh(float size, int repeatCount) {
    std::vector<std::unique_ptr<Mesh>> meshes;

    // 创建一个带有纹理坐标的平面作为地面
    vector<Vertex> vertices = {
        {
            // top right
            glm::vec3(size / 2.0f, 0.0f, size / 2.0f),   // Position
            glm::vec3(1.0f, 1.0f, 1.0f),                  // Color (白色，让纹理显示原色)
            glm::vec3(0.0f, 1.0f, 0.0f),                  // Normal (朝上)
            glm::vec2(repeatCount, repeatCount),          // TexCoords
        },
        {
            // bottom right
            glm::vec3(size / 2.0f, 0.0f, -size / 2.0f),  // Position
            glm::vec3(1.0f, 1.0f, 1.0f),                  // Color
            glm::vec3(0.0f, 1.0f, 0.0f),                  // Normal
            glm::vec2(repeatCount, 0.0f),                 // TexCoords
        },
        {
            // bottom left
            glm::vec3(-size / 2.0f, 0.0f, -size / 2.0f), // Position
            glm::vec3(1.0f, 1.0f, 1.0f),                  // Color
            glm::vec3(0.0f, 1.0f, 0.0f),                  // Normal
            glm::vec2(0.0f, 0.0f),                        // TexCoords
        },
        {
            // top left
            glm::vec3(-size / 2.0f, 0.0f, size / 2.0f),  // Position
            glm::vec3(1.0f, 1.0f, 1.0f),                  // Color
            glm::vec3(0.0f, 1.0f, 0.0f),                  // Normal
            glm::vec2(0.0f, repeatCount),                 // TexCoords
        },
    };
    
    vector<unsigned int> indices = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    auto mesh = std::make_unique<Mesh>(vertices, indices);
    mesh->SetDrawMode(GL_TRIANGLES);

    meshes.push_back(std::move(mesh));
    return meshes;
}
