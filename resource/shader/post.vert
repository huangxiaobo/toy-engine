#version 330 core

/*
 * 后处理全屏三角形顶点着色器
 *
 * 不加载任何顶点缓冲/属性：用 gl_VertexID 内建变量生成一个覆盖整个
 * NDC 的大三角形（业界标准做法，比全屏四边形少一个顶点且无插值接缝）：
 *   (-1,-1)、(3,-1)、(-1,3) 三个顶点恰好覆盖 [-1,1]² 屏幕区域。
 * uv 坐标由 NDC 线性映射到 [0,2]：[0,1] 为有效纹理范围，
 * 超出部分由纹理的 GL_CLAMP_TO_EDGE 钳制处理。
 */

out vec2 v2f_uv;

void main() {
    vec2 pos;
    if (gl_VertexID == 0) {
        pos = vec2(-1.0, -1.0);
    } else if (gl_VertexID == 1) {
        pos = vec2(3.0, -1.0);
    } else {
        pos = vec2(-1.0, 3.0);
    }
    v2f_uv = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}