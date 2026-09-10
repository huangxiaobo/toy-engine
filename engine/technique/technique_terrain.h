#ifndef __TECHNIQUE_TERRAIN_H__
#define __TECHNIQUE_TERRAIN_H__

// glad 必须在所有头文件之前（AGENTS.md 经验5）
#include <glad/gl.h>

#include "technique_light.h"

/*
 * 地形专用渲染技术（TechniqueTerrain）
 *
 * 职责：
 *   1. 封装地形专用着色器（terrain.vert / terrain.frag），继承 TechniqueLight
 *      以便复用方向/点/聚光灯光照与材质 uniform 的上传逻辑（terrain.frag 用到这些）
 *   2. 在基类之上新增地形专属的阴影采样管理：
 *      - 地面纹理采样（groundTexture → 单元0）
 *      - 阴影深度贴图统一绑定到纹理单元2，并在每次绘制时显式激活/绑定，
 *        避免依赖 Mesh 全局静态阴影状态链导致主 Pass 采样到错误的纹理
 *
 * 阴影映射涉及三套输入，本类在绘制时按固定纹理单元约定一次性激活/绑定：
 *   - lightSpace : 世界坐标 → 光源裁剪空间（顶点着色器算 FragPosLightSpace）
 *   - shadowMap  : 光源视角深度贴图采样器（片元着色器比较深度判影）
 *   - gUseShadow : 是否启用阴影（0=关闭，1=采样深度贴图）
 *
 * 纹理单元约定：单元0=地面漫反射纹理，单元2=阴影深度贴图（单元1 留给法线贴图启发）。
 * base Technique 已提供 SetLightSpaceMatrix / SetShadowMap / SetUniform，据此复用。
 */
class TechniqueTerrain : public TechniqueLight {
public:
    // name 用于日志标识；vertexShader/fragmentShader 为地形着色器文件路径
    TechniqueTerrain(std::string name, std::string vertexShader, std::string fragmentShader);
    ~TechniqueTerrain() override;

    // 设置地面漫反射纹理采样器（默认绑定到单元0），绘制前调用方可把地面纹理绑到单元0
    void SetGroundTexture(int unit = 0);

    /*
     * 记录本帧阴影启用状态、深度贴图纹理ID 与光源空间矩阵
     *
     * 由 Renderer 在阴影深度 Pass 结束后调用。真正把状态应用到 GPU 的
     * ApplyShadowState() 在绘制阶段（Enable 之后）执行。
     */
    void SetShadowState(bool enabled, unsigned int depthTexture, const glm::mat4 &lightSpace);

    /*
     * 绘制阶段应用阴影状态（必须在 Enable() 之后、glDrawElements 之前调用）
     *
     * 显式激活纹理单元2、重新绑定深度贴图，并上传 shadowMap/lightSpace/gUseShadow，
     * 防止其他对象绘制覆盖了单元2 的纹理绑定。
     */
    void ApplyShadowState();

private:
    // 是否启用阴影（0=关闭，1=启用），用于上传 gUseShadow
    unsigned int m_useShadow = 0;
    // 阴影深度贴图纹理ID（绘制时显式绑定到单元2）
    unsigned int m_depthTexture = 0;
    // 光源空间矩阵（世界坐标 → 光源裁剪空间）
    glm::mat4 m_lightSpace = glm::mat4(1.0f);
};

#endif // __TECHNIQUE_TERRAIN_H__
