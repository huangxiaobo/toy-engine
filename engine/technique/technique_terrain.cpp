// glad 必须在所有头文件之前
#include <glad/gl.h>

#include "technique_terrain.h"

#include "../shader/shader.h"
#include "../render_context.h"
#include <iostream>

/*
 * 地形专用渲染技术构造函数
 *
 * 构造基类 Technique 完成着色器加载/链接，并把类型标记为
 * TechniqueTypeLight（地形需要接收方向/点/聚光灯光照与阴影，走光照技术语义）。
 * 同时把地面纹理采样器与阴影采样器初始化为固定纹理单元。
 */
TechniqueTerrain::TechniqueTerrain(std::string name, std::string vertexShader, std::string fragmentShader)
    : TechniqueLight(std::move(name), std::move(vertexShader), std::move(fragmentShader)) {
    // TechniqueLight 构造已把 m_type 置为 TechniqueTypeLight 并初始化光照/材质 uniform，
    // 这里只需额外固定地面纹理采样器到单元0（阴影贴图见 ApplyShadowState 绑定单元2）
    SetGroundTexture(0);
}

TechniqueTerrain::~TechniqueTerrain() {
}

/*
 * 设置地面漫反射纹理采样器（groundTexture）绑定的纹理单元
 *
 * 仅通知着色器的 sampler 指向哪个单元，实际把地面纹理 bind 到该单元
 * 由绘制调用方负责（地形绘制前把地面纹理绑定到单元0）。
 */
void TechniqueTerrain::SetGroundTexture(int unit) {
    this->m_shader->SetUniformValue("groundTexture", unit);
}

/*
 * 绘制阶段应用阴影状态（必须在 Enable() 之后、glDrawElements 之前调用）
 *
 * 阴影状态每帧由 Renderer 组装进 RenderContext.shadow，本方法直接读取，
 * 与模型共用同一条状态通道。显式激活纹理单元2 并重新绑定深度贴图，
 * 然后上传 shadowMap 采样器、lightSpace 矩阵与 gUseShadow 开关。
 * 单元2 的反复显式绑定是防御性做法：当前渲染环其他对象（如天空穹/模型）
 * 的绘制可能覆盖了单元2 的纹理绑定，若依赖 Renderer 提前一次绑定，
 * 主 Pass 采样就会拿到错误的纹理。
 */
void TechniqueTerrain::ApplyShadowState(const ShadowState &shadow) {
    // 激活单元2 并绑定本帧的阴影深度贴图（未启用阴影时为0，解绑避免采样残留纹理）
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shadow.depthTexture);

    // 通知着色器 shadowMap 采样器指向单元2
    this->SetShadowMap(2);
    // 上传光源空间矩阵（世界坐标 → 光源裁剪空间）
    this->SetLightSpaceMatrix(shadow.lightSpace);
    // 上传阴影开关：沿用基类按名设置 int uniform
    this->SetUniform("gUseShadow", shadow.ready ? 1 : 0);
    // 上传 bias 缩放系数（gShadowBiasScale，默认 1.0 保持原公式效果）
    this->SetUniform("gShadowBiasScale", shadow.biasScale);
}
