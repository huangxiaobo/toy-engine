#ifndef __TECHNIQUE_H__
#define __TECHNIQUE_H__


#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class Shader;
class Light;
class Material;


// 渲染技术类型：语义轴正交的简单标记，供渲染器按能力甄别技术
// （当前仅 Base=无光照能力 / Light=支持光照材质），不承载"能力组合"推导。
// 阴影/深度 Pass 等能力通过 RenderContext.shadow 显式下发，不再走类型枚举分派。
enum TechniqueType {
    TechniqueTypeBase,
    TechniqueTypeLight,
};


class Technique {
protected:
    std::unique_ptr<Shader> m_shader;

    TechniqueType m_type;

    unsigned int m_uniform_projection;
    unsigned int m_uniform_view;
    unsigned int m_uniform_model;
    unsigned int m_uniform_wvp; // 模型视图投影矩阵
    unsigned int m_uniform_viewpos; // 摄像机位置
    unsigned int m_uniform_light_space = 0; // 光源空间矩阵(lightSpace) uniform 位置

public:
    Technique(const std::string &name, const std::string &vertexShader, const std::string &fragmentShader);

    virtual ~Technique();

    std::string Id; // Technique 唯一标识符（UUID，构造时自动生成）

    virtual TechniqueType GetType() const { return m_type; };

    Shader *GetShader() const;

    void SetWVPMatrix(const glm::mat4 &wvp);

    void SetCamera(const glm::vec3 &camera);

    void SetProjectionMatrix(const glm::mat4 &projection);

    void SetViewMatrix(const glm::mat4 &view);

    void SetModelMatrix(const glm::mat4 &model);

    void SetEyeWorldPos(const glm::vec3 &pos);

    void SetDirectionalLightTransform(const glm::mat4 &lvp);

    void SetUniform(const char *name, const glm::vec2 &value);

    void SetUniform(const char *name, const glm::vec3 &value);

    void SetUniform(const char *name, const glm::vec4 &value);

    void SetUniform(const char *name, float value);

    void SetUniform(const char *name, int value);

    void SetUniform(const char *name, const glm::mat4 &value);

    void SetUniform();

    void SetTextureUnit(unsigned int textureUnit);

    // 设置光源空间矩阵（lightSpace），供顶点着色器把世界坐标转换到光源视角做阴影判定
    void SetLightSpaceMatrix(const glm::mat4 &lightSpace);

    // 通知着色器：阴影深度贴图采样器绑定到哪个纹理单元
    void SetShadowMap(int unit);


    virtual void SetLights(const std::vector<Light *> &lights);

    virtual void SetMaterial(const Material *material);

    void Enable();

    void Disable();

public:
    static Technique *GetDefaultTechnique();
};

#endif
