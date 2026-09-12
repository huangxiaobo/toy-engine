#ifndef __SCENE_FRAMEBUFFER_H__
#define __SCENE_FRAMEBUFFER_H__

#include <glad/gl.h> // 必须在所有库的顶部（AGENTS.md 经验5）

/*
 * HDR 场景帧缓冲封装（多 Pass 渲染的第一步）
 *
 * 职责：承载所有 3D 场景绘制结果的离屏 FBO。
 *   - 颜色附件为 RGBA16F 浮点纹理：光照计算后的线性 HDR 值可无损保存，
 *     避免直接画进默认帧缓冲（无符号字节）时高光溢出/丢失。
 *   - 深度缓冲由离屏 Depth Renderbuffer 承担（场景对象深度测试用，不采样）。
 *   - 场景 Pass 之后，后处理 Pass 采样该颜色纹理做 tone mapping 再输出到默认缓冲。
 *
 * 视口约定（重要）：
 *   BindForWrite() 会保存当前视口并切换到本 FBO 分辨率，
 *   Unbind() 恢复之前保存的视口——遵循 AGENTS.md 中
 *   "修改全局 OpenGL 状态必须保存和恢复"的教训，调用方无需自己处理视口。
 */
class SceneFramebuffer {
public:
    SceneFramebuffer() = default;
    ~SceneFramebuffer();

    // 创建指定分辨率的 HDR 颜色纹理与 FBO；分辨率变化时内部先释放旧资源再重建
    void Init(int width, int height);

    // 绑定 FBO 并清空颜色/深度缓冲，准备写入场景 Pass；内部保存并切换视口
    void BindForWrite();
    // 解绑 FBO 回到默认帧缓冲，并恢复 BindForWrite 时保存的视口
    void Unbind();

    // 获取 HDR 颜色纹理（供后处理 Pass 采样）
    unsigned int GetColorTexture() const { return m_colorTex; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    void Destroy();

    unsigned int m_fbo = 0;
    // HDR 线性颜色纹理（RGBA16F，后处理 Pass 采样它）
    unsigned int m_colorTex = 0;
    // 离屏深度 Renderbuffer，仅供场景 Pass 深度测试（不直接采样）
    unsigned int m_depthRbo = 0;
    // BindForWrite 时保存的主视口（Unbind 时恢复）；[x, y, width, height]
    int m_savedViewport[4] = {0, 0, 0, 0};
    int m_width = 0;
    int m_height = 0;
};

#endif // __SCENE_FRAMEBUFFER_H__