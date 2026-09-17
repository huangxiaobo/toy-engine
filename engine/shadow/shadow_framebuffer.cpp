#include <glad/gl.h> // 必须在所有库的顶部

#include "shadow_framebuffer.h"
#include <iostream>

ShadowFramebuffer::~ShadowFramebuffer() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_depthTex != 0) {
        glDeleteTextures(1, &m_depthTex);
        m_depthTex = 0;
    }
    if (m_depthRbo != 0) {
        glDeleteRenderbuffers(1, &m_depthRbo);
        m_depthRbo = 0;
    }
}

/*
 * 创建阴影深度贴图与帧缓冲
 *
 * 步骤：
 *   1. 生成一张 R32F 颜色纹理用于保存深度值（主 Pass 用它做手动深度比较）。
 *      这里故意不用 GL_DEPTH_COMPONENT 深度纹理，因为 macOS 上 sampler2D 采样
 *      GL_DEPTH_COMPONENT 并读 .r 不稳定（常返回 0），R32F 颜色附件采样 .r 才稳定。
 *   2. 深度缓冲本身交给一张离屏 Depth Renderbuffer（m_depthRbo）承担，
 *      它只用于深度 Pass 的深度测试，不直接采样。深度值与写入 R32F 的
 *      gl_FragCoord.z 完全一致。
 *   3. 不开启 GL_TEXTURE_COMPARE_MODE（保持默认 GL_NONE）：着色器采用"手动比较"
 *      （ShadowCalculation：取 texture().r 原始深度，再与当前片元深度做
 *      currentDepth - bias > closestDepth 判定）。若开启渐进式采样比较，
 *      texture() 返回 0/1 遮挡标志而非原始深度，手动比较会对上 0/1 导致阴影错乱。
 *   4. 采样环绕设为 GL_CLAMP_TO_BORDER 且边界为白色(1.0)，
 *      使超出阴影贴图范围(如远景)视为"被照亮"，而不是误判为阴影。
 *   5. 主 Pass 的 currentDepth 由 lightSpace 变换得 projCoords.z，
 *      与该 R32F 里存的 gl_FragCoord.z 同源，二者天然一致（仅差 polygon offset）。
 */
void ShadowFramebuffer::Init(int width, int height) {
    m_width = width;
    m_height = height;

    // 1. 创建 R32F 颜色纹理（保存深度值，主 Pass 采样它）
    glGenTextures(1, &m_depthTex);
    glBindTexture(GL_TEXTURE_2D, m_depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_width, m_height,
                 0, GL_RED, GL_FLOAT, nullptr);

    // 深度贴图不应做线性过滤，否则边缘深度会被插值产生错误阴影边界
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // 超出贴图范围(如远景)返回 1.0(最远)，代表无遮挡、被照亮
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // 保持默认的 GL_NONE 采样模式：着色器手动比较获取原始深度（见上方注释说明）

    // 2. 创建离屏 Depth Renderbuffer，仅供深度 Pass 做深度测试
    glGenRenderbuffers(1, &m_depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // 3. 创建 FBO：R32F 颜色附件 + Depth Renderbuffer 附件
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_depthTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);
    // 显式声明写入目标为颜色附件 0（默认即为此值，显式声明更明确）
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 4. 校验 FBO 完整性
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ShadowFramebuffer: FBO is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*
 * 绑定阴影 FBO 准备写入
 *
 * 将视口切换到阴影贴图分辨率并清空颜色(R32F)与深度缓冲。
 * 调用方在完成深度 Pass 后必须调用 Unbind() 并恢复主视口大小。
 */
void ShadowFramebuffer::BindForWrite() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
    // 颜色附件存深度值，深度附件做深度测试，两者都要清
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // 未写入处保留为"最远"，避免误判阴影
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ShadowFramebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
