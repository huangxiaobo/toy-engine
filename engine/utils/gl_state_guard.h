#ifndef __GL_STATE_GUARD_H__
#define __GL_STATE_GUARD_H__

// glad 必须第一个包含：GLuint/GLenum 等类型由此提供
#include <glad/gl.h>

/*
 * OpenGL 状态守卫（RAII）
 *
 * 本项目踩过的坑：绘制函数改了全局 GL 状态却不恢复，导致后续对象渲染错误
 * （见 AGENTS.md 教训 2：ParticleSystem 泄漏 GL_CULL_FACE，天空球直接不可见）。
 * 逐处手工 glGet 保存 + glDisable 恢复很容易漏写其中一项，这里让配对变成结构性的。
 *
 * 按关注点拆分而不是做一个"全状态"万能守卫：每个 guard 只保存自己负责的那一项，
 * 避免每帧多余的 glGet 查询。需要恢复多项时在同一作用域声明多个 guard 组合即可。
 *
 * 用法：
 *     {
 *         GLBlendGuard blend;          // 析构时自动恢复混合开关
 *         GLDepthWriteGuard depth;     // 可组合声明，作用域结束逐个恢复
 *         glEnable(GL_BLEND);
 *         glDepthMask(GL_FALSE);
 *         ...                           // 任意提前 return / 异常都不会泄漏状态
 *     }
 *
 * 不适用范围：帧缓冲绑定的恢复要跨函数调用（BindForWrite 与 Unbind 分离），
 * 作用域守卫无法跨越，故 FBO 类各自持有持久保存槽，不在本文件提供。
 */

/* 视口。3D 绘制期间切到中央节点矩形，结束后须原样恢复，
 * 否则 ImGui 面板会画在 3D 视口偏移之下 */
class GLViewportGuard {
public:
    GLViewportGuard();
    ~GLViewportGuard();

    GLViewportGuard(const GLViewportGuard &) = delete;
    GLViewportGuard &operator=(const GLViewportGuard &) = delete;

private:
    GLint m_prev[4] = {0, 0, 0, 0};
};

/* 多边形光栅化模式（实心 / 线框）。本项目只整体设置 FRONT_AND_BACK，故取双值一起存 */
class GLPolygonModeGuard {
public:
    GLPolygonModeGuard();
    ~GLPolygonModeGuard();

    GLPolygonModeGuard(const GLPolygonModeGuard &) = delete;
    GLPolygonModeGuard &operator=(const GLPolygonModeGuard &) = delete;

private:
    GLint m_prev[2] = {GL_FILL, GL_FILL};
};

/* 深度测试开关（不影响深度写入掩码，后者见 GLDepthWriteGuard） */
class GLDepthTestGuard {
public:
    GLDepthTestGuard();
    ~GLDepthTestGuard();

    GLDepthTestGuard(const GLDepthTestGuard &) = delete;
    GLDepthTestGuard &operator=(const GLDepthTestGuard &) = delete;

private:
    GLboolean m_prev = GL_FALSE;
};

/* 深度写入掩码。半透明绘制常关掉它，但深度测试仍开着，两者要分开管 */
class GLDepthWriteGuard {
public:
    GLDepthWriteGuard();
    ~GLDepthWriteGuard();

    GLDepthWriteGuard(const GLDepthWriteGuard &) = delete;
    GLDepthWriteGuard &operator=(const GLDepthWriteGuard &) = delete;

private:
    GLboolean m_prev = GL_TRUE;
};

/* 混合开关 */
class GLBlendGuard {
public:
    GLBlendGuard();
    ~GLBlendGuard();

    GLBlendGuard(const GLBlendGuard &) = delete;
    GLBlendGuard &operator=(const GLBlendGuard &) = delete;

private:
    GLboolean m_prev = GL_FALSE;
};

/* 混合函数（源/目的混合因子）。开关之外的另一半混合状态：
 * 粒子用加法混合，漏掉它会让后续所有半透明绘制继承加法混合 */
class GLBlendFuncGuard {
public:
    GLBlendFuncGuard();
    ~GLBlendFuncGuard();

    GLBlendFuncGuard(const GLBlendFuncGuard &) = delete;
    GLBlendFuncGuard &operator=(const GLBlendFuncGuard &) = delete;

private:
    GLint m_prev_src = GL_ONE;
    GLint m_prev_dst = GL_ZERO;
};

/* 背面剔除开关 */
class GLCullFaceGuard {
public:
    GLCullFaceGuard();
    ~GLCullFaceGuard();

    GLCullFaceGuard(const GLCullFaceGuard &) = delete;
    GLCullFaceGuard &operator=(const GLCullFaceGuard &) = delete;

private:
    GLboolean m_prev = GL_FALSE;
};

/* 面片深度偏移。除开关外还须存回 factor/units 两个参数：
 * 若进入时偏移本就处于启用状态，只恢复开关而不恢复参数仍会改变渲染结果 */
class GLPolygonOffsetGuard {
public:
    GLPolygonOffsetGuard();
    ~GLPolygonOffsetGuard();

    GLPolygonOffsetGuard(const GLPolygonOffsetGuard &) = delete;
    GLPolygonOffsetGuard &operator=(const GLPolygonOffsetGuard &) = delete;

private:
    GLboolean m_prev = GL_FALSE;
    GLfloat m_prev_factor = 0.0f;
    GLfloat m_prev_units = 0.0f;
};

/* GL_PROGRAM_POINT_SIZE 开关：点大小取自着色器还是 gl_PointSize */
class GLProgramPointSizeGuard {
public:
    GLProgramPointSizeGuard();
    ~GLProgramPointSizeGuard();

    GLProgramPointSizeGuard(const GLProgramPointSizeGuard &) = delete;
    GLProgramPointSizeGuard &operator=(const GLProgramPointSizeGuard &) = delete;

private:
    GLboolean m_prev = GL_FALSE;
};

/* 当前活动纹理单元 */
class GLActiveTextureGuard {
public:
    GLActiveTextureGuard();
    ~GLActiveTextureGuard();

    GLActiveTextureGuard(const GLActiveTextureGuard &) = delete;
    GLActiveTextureGuard &operator=(const GLActiveTextureGuard &) = delete;

private:
    GLint m_prev = GL_TEXTURE0;
};

#endif // __GL_STATE_GUARD_H__
