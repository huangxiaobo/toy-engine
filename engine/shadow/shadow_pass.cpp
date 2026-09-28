#include <glad/gl.h> // 必须在所有库的顶部

#include "shadow_pass.h"
#include "shadow_framebuffer.h"
#include "../technique/technique.h"
#include "../render_context.h"
#include "../utils/gl_state_guard.h"
#include "../model/model.h"
#include "../mesh/mesh.h"
#include "../terrain/terrain_manager.h"
#include "../light/light.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace std;

ShadowPass::ShadowPass() {
}

ShadowPass::~ShadowPass() {
}

void ShadowPass::Init() {
    // 阴影贴图分辨率 2048×2048：越高越清晰但越耗显存/带宽
    m_fbo = std::make_unique<ShadowFramebuffer>();
    m_fbo->Init(2048, 2048);
    // 深度 Pass 专用着色器，由本类独占生命周期
    m_depth_tech = std::make_unique<Technique>("shadow_depth",
                                              "./resource/shader/depth.vert",
                                              "./resource/shader/depth.frag");
}

unsigned int ShadowPass::GetDepthTexture() const {
    return m_fbo->GetDepthTexture();
}

void ShadowPass::Render(const vector<unique_ptr<Model>> &models,
                        const TerrainManager *terrain,
                        RenderContext &ctx) {
    m_map_ready = false;
    // 每帧先重置光源摄像机参数的有效标志：仅当本帧存在已启用的方向光时才置为有效
    m_camera.m_available = false;

    if (!m_enabled || m_fbo == nullptr || m_depth_tech == nullptr) {
        return;
    }

    DirectionLight *dirLight = nullptr;
    for (auto light: ctx.m_lights) {
        if (light->GetLightType() == LightTypeDirection && light->IsEnabled()) {
            dirLight = static_cast<DirectionLight *>(light);
            break;
        }
    }
    if (dirLight == nullptr) {
        return;
    }

    /*
     * 计算光源空间矩阵：平行光用正交投影（覆盖场景范围），保证阴影精度与覆盖平衡。
     * 光照计算用 toLight = -Direction（光源在 -Direction 方向远处，即场景上方），
     * 因此深度 Pass 的光源"摄像机"也必须放在 -Direction 一侧并朝场景看，
     * 否则深度贴图从相反方向生成，深度比较永远对不齐光照，阴影无法显示。
     *
     * 正交投影范围按场景 AABB 自适应（包围球半径 + 10% 边距 + 5 单位）：
     * 固定 ±210 覆盖全部地形时，sphere 直径 3 单位仅占 ~0.7% 像素、阴影明显像素化；
     * 收紧到约 ±80 后同分辨率下阴影精度提升近 3 倍，PCF 软阴影也更细腻。
     * 近/远平面沿用 1/100：光源在场景中心上方 30 单位，最远角点距光源约 80，足够。
     */
    glm::vec3 lightDir = glm::normalize(dirLight->m_direction);
    m_camera.m_direction = lightDir;

    // 场景包围盒 → 中心 + 包围球半径，光源正交盒以 中心 ±(半径+边距) 覆盖。
    // 用包围球而非 AABB 是因为光源视图有朝向旋转，球在任意旋转下都被正方形盒包含
    glm::vec3 sceneMin, sceneMax;
    computeSceneBounds(models, terrain, sceneMin, sceneMax);
    const glm::vec3 sceneCenter = (sceneMin + sceneMax) * 0.5f;
    const float sceneRadius = glm::length(sceneMax - sceneCenter);
    const float span = sceneRadius * 1.1f + 5.0f; // 10% 边距 + 5 单位余量

    m_camera.m_look_at = sceneCenter;
    m_camera.m_position = sceneCenter - lightDir * 30.0f; // 光源位于场景上方（-Direction 远处），向下照射
    m_camera.m_ortho_left = -span;
    m_camera.m_ortho_right = span;
    m_camera.m_ortho_bottom = -span;
    m_camera.m_ortho_top = span;

    /*
     * 当光源恰好垂直位于场景正上方（如太阳方向 (0,-1,0) 纯直下）时，lookAt 的
     * look 向量与默认 up=(0,1,0) 完全平行，cross 得零向量，normalize 产生 NaN
     * 导致所有顶点深度无效、深度贴图为空。此时改用 Z 轴作 up 避免退化。
     *
     * 【不要改回 X 轴】曾用 X 轴(1,0,0) 作 up，导致光源视图坐标系发生 90° 旋转
     * （世界 z → 光源 x、世界 x → 光源 y），阴影贴图采样坐标轴互换，表现为纯垂直光下
     * 球体出现"沿 z=0 经线"的红/黑异常分界。改用 Z 轴后光源视图 x 轴 = 世界 x、
     * y 轴 = 世界 z，深度仍沿 -y，分界恢复为正确的水平纬线（顶部亮/底部暗）。
     */
    glm::vec3 lookDir = glm::normalize(m_camera.m_look_at - m_camera.m_position);
    glm::vec3 safeUp = (fabs(lookDir.y) > 0.999f)
                         ? glm::vec3(0.0f, 0.0f, 1.0f)
                         : glm::vec3(0.0f, 1.0f, 0.0f);
    m_camera.m_up = safeUp;

    // 视图/投影矩阵统一由 m_camera 字段构建（字段是唯一参数来源）
    m_camera.m_light_view = glm::lookAt(m_camera.m_position, m_camera.m_look_at, m_camera.m_up);
    m_camera.m_light_projection = glm::ortho(m_camera.m_ortho_left, m_camera.m_ortho_right,
                                          m_camera.m_ortho_bottom, m_camera.m_ortho_top,
                                          m_camera.m_near_plane, m_camera.m_far_plane);
    m_camera.m_available = true;

    const glm::mat4 lightSpace = m_camera.m_light_projection * m_camera.m_light_view;

    // 主视口与帧缓冲绑定由 ShadowFramebuffer::BindForWrite/Unbind 负责存取，无需在此手工保存
    m_fbo->BindForWrite();

    // 置位 passActive 后 Mesh::Draw 会改走"只写深度"分支
    ctx.m_shadow.m_depth_tech = m_depth_tech.get();
    ctx.m_shadow.m_light_space = lightSpace;
    ctx.m_shadow.m_pass_active = true;

    /*
     * 深度 Pass 开启面片深度偏移(GL_POLYGON_OFFSET_FILL)：把写入阴影贴图的深度
     * 统一推远，给主绘制采样比较留出余量，从根本上消除平坦地面在倾斜方向光下的
     * 自阴影痤疮(acne)，且不依赖着色器内超大 bias（超大 bias 会让 bunny 等模型
     * 的阴影"飘浮/peter-panning"）。退出时释放 guard，连同 factor/units 一起恢复。
     */
    std::unique_ptr<GLPolygonOffsetGuard> polygon_offset;
    polygon_offset = std::make_unique<GLPolygonOffsetGuard>();
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    for (const auto &m: models) {
        m->Draw(ctx, glm::mat4(1.0f));
    }

    polygon_offset.reset();
    ctx.m_shadow.m_pass_active = false;
    // Unbind 会恢复进入阴影 Pass 前的帧缓冲绑定与主视口
    m_fbo->Unbind();

    // 阴影深度贴图绑定到纹理单元2（单元0/1 已被漫反射/法线贴图占用），
    // 后续每个 Mesh::Draw 会通过 SetShadowMap(2) 通知着色器采样它
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_fbo->GetDepthTexture());

    ctx.m_shadow.m_ready = true;
    ctx.m_shadow.m_depth_texture = m_fbo->GetDepthTexture();
    m_map_ready = true;
}

void ShadowPass::computeSceneBounds(const vector<unique_ptr<Model>> &models,
                                    const TerrainManager *terrain,
                                    glm::vec3 &outMin, glm::vec3 &outMax) const {
    // 先以地形地面范围为基准（原点为中心 planeSize×planeSize，高度 0~heightScale），
    // 再并入所有模型网格世界坐标范围；两者都没有时回退旧默认 ±210 保证阴影覆盖取景范围
    bool haveBounds = false;

    if (terrain != nullptr) {
        const TerrainConfig &cfg = terrain->GetConfig();
        const float half = cfg.m_plane_size * 0.5f;
        outMin = glm::vec3(-half, 0.0f, -half);
        outMax = glm::vec3(half, cfg.m_height_scale, half);
        haveBounds = true;
    }

    for (const auto &model: models) {
        const glm::mat4 world = model->GetWorldMatrix();
        for (const auto &mesh: model->GetMeshes()) {
            for (const auto &vertex: mesh->m_vertices) {
                const glm::vec3 p = glm::vec3(world * glm::vec4(vertex.m_position, 1.0f));
                if (!haveBounds) {
                    outMin = outMax = p;
                    haveBounds = true;
                } else {
                    outMin = glm::min(outMin, p);
                    outMax = glm::max(outMax, p);
                }
            }
        }
    }

    if (!haveBounds) {
        outMin = glm::vec3(-210.0f);
        outMax = glm::vec3(210.0f);
    }
}
