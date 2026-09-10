#version 330 core

/*
 * 深度 Pass 片元着色器
 *
 * FBO 上挂了两类附件（macOS 兼容方案，见 shadow_framebuffer.cpp）：
 *   1. 一张 R32F 颜色附件，用来"保存深度值"，主 Pass 会采样它做手动深度比较。
 *   2. 一张 Depth Renderbuffer，仅供本深度 Pass 做深度测试。
 *
 * 这里必须把当前片元深度 gl_FragCoord.z（已由视口映射到 [0,1]，与光源光空间的
 * projCoords.z 同源）写进 R32F 颜色附件，否则 R32F 颜色附件不会被填充，
 * 主 Pass 采样到的 closestDepth 恒为 0，导致整帧被误判成阴影。
 */

out float depthColor;

void main() {
    // 只关心深度值：把片元深度写入 R32F 颜色附件（.r 通道即保存的深度）
    depthColor = gl_FragCoord.z;
}
