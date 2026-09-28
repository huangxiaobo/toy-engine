#include <glad/gl.h> // 必须在所有库的顶部

#include "post_process_pass.h"
#include "../technique/technique.h"
#include "../utils/gl_state_guard.h"

PostProcessPass::~PostProcessPass() {
    if (m_post_vao != 0) {
        glDeleteVertexArrays(1, &m_post_vao);
        m_post_vao = 0;
    }
}

void PostProcessPass::Init() {
    m_post_tech = std::make_unique<Technique>("post",
                                             "./resource/shader/post.vert",
                                             "./resource/shader/post.frag");
    glGenVertexArrays(1, &m_post_vao);
}

void PostProcessPass::Render(unsigned int scene_color_texture) {
    // 后处理是 2D 全屏操作，不需要深度测试与混合；guard 随本函数返回自动恢复
    GLDepthTestGuard depth_test;
    GLBlendGuard blend;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_post_tech->Enable();
    m_post_tech->SetUniform("sceneTex", 0);
    m_post_tech->SetUniform("toneMapMode", m_tone_mapping_enabled ? 1 : 0);
    m_post_tech->SetUniform("exposure", m_exposure);
    m_post_tech->SetUniform("saturation", m_saturation);
    m_post_tech->SetUniform("contrast", m_contrast);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_color_texture);

    glBindVertexArray(m_post_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);   // 全屏三角形：3 个顶点覆盖整个视口
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
