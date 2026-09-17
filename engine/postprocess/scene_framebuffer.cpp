#include <glad/gl.h> // 必须在所有库的顶部

#include "scene_framebuffer.h"
#include <iostream>

SceneFramebuffer::~SceneFramebuffer() {
    Destroy();
}

/*
 * 创建 HDR 场景帧缓冲
 *
 * 步骤：
 *   1. 颜色附件 RGBA16F：保存线性 HDR 光照结果，供后处理 Pass 采样做 tone mapping。
 *      过滤用 GL_LINEAR（后处理放大/缩小时平滑），环绕用 GL_CLAMP_TO_EDGE
 *      （全屏三角形采样 uv 会超出 [0,1]，钳制到边缘避免 REPEAT 带来的接缝）。
 *   2. 深度缓冲交给离屏 Depth Renderbuffer（场景对象深度测试用，不采样）。
 *   3. 显式声明写入目标为颜色附件 0。
 *   4. Init 幂等：分辨率变化时先释放旧资源再重建，供 resize/每帧同步视口尺寸时复用。
 */
void SceneFramebuffer::Init(int width, int height) {
    // 尺寸未变化时跳过，避免每帧重复重建纹理/FBO（mainwindow 每帧都会调一次 resize）
    if (m_fbo != 0 && m_width == width && m_height == height) {
        return;
    }
    Destroy();

    m_width = width;
    m_height = height;

    // 1. 创建 RGBA16F HDR 颜色纹理（后处理 Pass 采样它做 tone mapping）
    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height,
                 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 2. 创建离屏 Depth Renderbuffer，仅供场景 Pass 做深度测试
    glGenRenderbuffers(1, &m_depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // 3. 创建 FBO：HDR 颜色附件 + Depth Renderbuffer 附件
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 4. 校验 FBO 完整性
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SceneFramebuffer: FBO is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*
 * 绑定场景 FBO 准备写入
 *
 * 保存当前视口（mainwindow 设置的中央视口），切换到 FBO 分辨率并清空
 * 颜色（HDR 下用黑色，场景天空会覆盖它）与深度缓冲。
 * 调用方绘制完场景后必须调用 Unbind() 回到默认帧缓冲。
 */
void SceneFramebuffer::BindForWrite() {
    glGetIntegerv(GL_VIEWPORT, m_savedViewport);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SceneFramebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // 恢复 BindForWrite 时保存的主视口，保证后续绘制（后处理 Pass、ImGui）位置正确
    glViewport(m_savedViewport[0], m_savedViewport[1], m_savedViewport[2], m_savedViewport[3]);
}

void SceneFramebuffer::Destroy() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_colorTex != 0) {
        glDeleteTextures(1, &m_colorTex);
        m_colorTex = 0;
    }
    if (m_depthRbo != 0) {
        glDeleteRenderbuffers(1, &m_depthRbo);
        m_depthRbo = 0;
    }
    m_width = 0;
    m_height = 0;
}