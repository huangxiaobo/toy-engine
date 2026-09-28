#include "gl_state_guard.h"

// 全部 guard 的模式一致：构造时 glGet 保存当前值，析构时写回。
// 提前 return 或异常展开都会触发析构，状态不会泄漏给后续绘制。

GLViewportGuard::GLViewportGuard() {
    glGetIntegerv(GL_VIEWPORT, m_prev);
}

GLViewportGuard::~GLViewportGuard() {
    glViewport(m_prev[0], m_prev[1], m_prev[2], m_prev[3]);
}

GLPolygonModeGuard::GLPolygonModeGuard() {
    glGetIntegerv(GL_POLYGON_MODE, m_prev);
}

GLPolygonModeGuard::~GLPolygonModeGuard() {
    glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(m_prev[0]));
}

GLDepthTestGuard::GLDepthTestGuard() {
    m_prev = glIsEnabled(GL_DEPTH_TEST);
}

GLDepthTestGuard::~GLDepthTestGuard() {
    if (m_prev) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

GLDepthWriteGuard::GLDepthWriteGuard() {
    glGetBooleanv(GL_DEPTH_WRITEMASK, &m_prev);
}

GLDepthWriteGuard::~GLDepthWriteGuard() {
    glDepthMask(m_prev);
}

GLBlendGuard::GLBlendGuard() {
    m_prev = glIsEnabled(GL_BLEND);
}

GLBlendGuard::~GLBlendGuard() {
    if (m_prev) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}

GLBlendFuncGuard::GLBlendFuncGuard() {
    glGetIntegerv(GL_BLEND_SRC, &m_prev_src);
    glGetIntegerv(GL_BLEND_DST, &m_prev_dst);
}

GLBlendFuncGuard::~GLBlendFuncGuard() {
    glBlendFunc(static_cast<GLenum>(m_prev_src), static_cast<GLenum>(m_prev_dst));
}

GLCullFaceGuard::GLCullFaceGuard() {
    m_prev = glIsEnabled(GL_CULL_FACE);
}

GLCullFaceGuard::~GLCullFaceGuard() {
    if (m_prev) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

GLPolygonOffsetGuard::GLPolygonOffsetGuard() {
    m_prev = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &m_prev_factor);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &m_prev_units);
}

GLPolygonOffsetGuard::~GLPolygonOffsetGuard() {
    glPolygonOffset(m_prev_factor, m_prev_units);
    if (m_prev) {
        glEnable(GL_POLYGON_OFFSET_FILL);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

GLProgramPointSizeGuard::GLProgramPointSizeGuard() {
    m_prev = glIsEnabled(GL_PROGRAM_POINT_SIZE);
}

GLProgramPointSizeGuard::~GLProgramPointSizeGuard() {
    if (m_prev) {
        glEnable(GL_PROGRAM_POINT_SIZE);
    } else {
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
}

GLActiveTextureGuard::GLActiveTextureGuard() {
    glGetIntegerv(GL_ACTIVE_TEXTURE, &m_prev);
}

GLActiveTextureGuard::~GLActiveTextureGuard() {
    glActiveTexture(static_cast<GLenum>(m_prev));
}
