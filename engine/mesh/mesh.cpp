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

// ---- 阴影深度 Pass 的全局状态（文件内静态，由 Renderer 每帧通过 SetShadowDepthState 切换） ----
// 采用"全局状态 + Mesh::Draw 短路"的方式，让所有会投阴影的几何体（地形、模型）
// 无需逐个配置，即可在深度 Pass 中复用同一套绘制路径（几何与主渲染完全一致，阴影不会错位）。
static Technique *s_shadowDepthTech = nullptr;      // 深度 Pass 专用着色器（depth.vert/depth.frag）
static glm::mat4 s_shadowLightSpace = glm::mat4(1.0f); // 光源空间矩阵（正交投影 * 光源视图）
static bool s_shadowPassActive = false;             // 当前是否处于阴影深度 Pass
static bool s_shadowMapReady = false;               // 本帧是否存在已生成的阴影深度贴图

/*
 * 切换阴影深度 Pass 状态
 *
 * Renderer 在每帧深度 Pass 前调用 active=true 绑定深度着色器与 lightSpace，
 * 完成后调用 active=false 恢复正常光照绘制。
 * tech 为空时不启用深度 Pass（安全兜底，避免空指针）。
 * 注意：s_shadowMapReady 由 SetShadowMapAvailable 单独控制，与 s_shadowPassActive
 * 相互独立——前者表示"已有可采样的阴影贴图"，后者表示"正在写阴影贴图"。
 */
void Mesh::SetShadowDepthState(Technique *tech, const glm::mat4 &lightSpace, bool active) {
    s_shadowDepthTech = tech;
    // 仅在进入阴影深度 Pass 时更新 lightSpace 矩阵；
    // 退出时（active=false）保留已有的正确值，供主 Pass 绘制地形/模型时采样阴影使用。
    // 旧实现在退出时用单位矩阵覆盖，导致地形 FragPosLightSpace = I * WorldPos0，
    // projCoords 范围 [-4.5, 5.5] 远超 [0,1]，阴影完全失效。
    if (active) {
        s_shadowLightSpace = lightSpace;
    }
    s_shadowPassActive = active && (tech != nullptr);

    // 深度 Pass 渲染时开启面片深度偏移(GL_POLYGON_OFFSET_FILL)：
    // 把写入阴影贴图的深度统一推远，保证与主绘制采样做比较时留出余量，
    // 从根本上消除平坦地面在倾斜方向光下的自阴影痤疮(acne)，且不依赖着色器内超大 bias
    // （超大 bias 会让 bunny 等模型的阴影"飘浮/peter-panning"）。
    // 结束后必须关闭该 GL 状态，避免泄漏影响后续场景绘制。
    if (s_shadowPassActive) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

/*
 * 设置本帧是否已有可采样的阴影深度贴图
 *
 * 主渲染 Pass 的 Mesh::Draw 仅在 s_shadowMapReady 为 true 时才上传 lightSpace /
 * 通知着色器绑定 shadowMap；若阴影被禁用（无方向光/开关关闭），则不采样，
 * 避免采样到未绑定的纹理导致画面整体变暗。
 */
void Mesh::SetShadowMapAvailable(bool available) {
    s_shadowMapReady = available;
}

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

void Mesh::Draw(long long elapsed, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model,
                const glm::vec3 &camera, const std::vector<Light *> &lights) {
    // 阴影深度 Pass：切换到光源视角后，本函数被再次调用以渲染阴影贴图。
    // 此时只写深度、不做光照计算，绘制同样的几何但用 gDepthMVP(=lightSpace*model) 变换。
    // 提前 return 防止走到下方的光照 Pass（会污染深度贴图或浪费性能）。
    if (s_shadowPassActive) {
        if (s_shadowDepthTech == nullptr) {
            return;
        }
        s_shadowDepthTech->Enable();
        s_shadowDepthTech->SetUniform("gDepthMVP", s_shadowLightSpace * model);
        /* 重新绑定 VAO */
        glBindVertexArray(VAO);
        glDrawElements(DrawMode, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, (void *) 0);
        glBindVertexArray(0);
        return;
    }

    this->m_effect->Enable();
    this->m_effect->SetProjectionMatrix(projection);
    this->m_effect->SetViewMatrix(view);
    this->m_effect->SetModelMatrix(model);
    // this->m_effect->SetWVPMatrix(mvp);
    this->m_effect->SetCamera(camera);

    this->m_effect->SetLights(lights);

    // 阴影贴图相关 uniform：
    //   - gUseShadow 始终设置：阴影可用时为1（着色器据此启用阴影计算），否则为0
    //   - 仅当阴影贴图存在时才上传 lightSpace / 绑定 shadowMap，
    //     避免在阴影被禁用时让着色器采样未绑定纹理导致画面整体变暗
    this->m_effect->SetUniform("gUseShadow", s_shadowMapReady ? 1 : 0);
    if (s_shadowMapReady) {
        this->m_effect->SetShadowMap(2);
        this->m_effect->SetLightSpaceMatrix(s_shadowLightSpace);
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
