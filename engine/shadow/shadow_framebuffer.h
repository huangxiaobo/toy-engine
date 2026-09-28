#ifndef __SHADOW_FRAMEBUFFER_H__
#define __SHADOW_FRAMEBUFFER_H__

/*
 * 阴影贴图硬件缓冲封装
 *
 * 为方向光阴影映射提供一个"深度写入 + 深度可采样"的帧缓冲(FBO)：
 *   - 深度 Pass 把场景从光源视角写入这张"深度贴图"
 *   - 主渲染 Pass 采样该深度贴图，配合 lightSpace 矩阵判断片元是否处于阴影中
 *
 * 存储方案（重要，macOS 兼容）：
 *   深度贴图使用 GL_R32F 颜色附件纹理来保存深度值，而不是 GL_DEPTH_COMPONENT 深度纹理。
 *   因为在 macOS 的 OpenGL 驱动上，用普通 sampler2D 采样 GL_DEPTH_COMPONENT 深度纹理并读取
 *   .r 通道（手动深度比较）并不被稳定支持，常返回 0，导致整帧被误判为阴影（也就是之前
 *   closestDepth 恒为 0 的根因）。改为把深度值写入 R32F 颜色附件后，sampler2D 采样 .r 即可
 *   稳定拿到原始深度。深度缓冲本身仍由一张离屏 Depth Renderbuffer 承担（用于深度测试），
 *   深度值与采样到的颜色值一致（都由 gl_FragCoord.z 产生）。
 *
 * 注意：BindForWrite/Unbind 必须成对调用，Unbind 恢复主视口与原帧缓冲绑定
 * （与 SceneFramebuffer 保持一致，调用方无需再手工存取视口）。
 */

#include <glad/gl.h> // 必须在所有库的顶部

class ShadowFramebuffer {
public:
    ShadowFramebuffer() = default;
    ~ShadowFramebuffer();
    ShadowFramebuffer(const ShadowFramebuffer &) = delete;
    ShadowFramebuffer &operator=(const ShadowFramebuffer &) = delete;

    // 创建指定分辨率的深度贴图与 FBO（分辨率越高阴影越清晰、越耗显存/性能）
    void Init(int width, int height);

    // 绑定 FBO 并清空深度缓冲，准备写入阴影深度 Pass
    void BindForWrite();
    // 恢复 BindForWrite 之前的帧缓冲绑定与主视口
    void Unbind();

    // 获取深度纹理 ID（R32F 颜色附件），供主 Pass 采样（绑定到纹理单元）
    unsigned int GetDepthTexture() const { return m_depth_tex; }

private:
    unsigned int m_fbo = 0;
    // 保存深度值的 R32F 颜色纹理（主 Pass 采样它）
    unsigned int m_depth_tex = 0;
    // 离屏深度 Renderbuffer，仅供深度 Pass 做深度测试（不直接采样）
    unsigned int m_depth_rbo = 0;
    int m_width = 0;
    int m_height = 0;
    // BindForWrite 前的视口，Unbind 时原样写回
    GLint m_saved_viewport[4] = {0, 0, 0, 0};
    // BindForWrite 前的帧缓冲绑定，Unbind 时原样写回
    GLint m_saved_framebuffer = 0;
};

#endif // __SHADOW_FRAMEBUFFER_H__
