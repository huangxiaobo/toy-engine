#ifndef __TECHNIQUE_LIGHT_H__
#include <glad/gl.h> //  必须在所有库的顶部

#include "technique.h"

class DirectionLight;
class PointLight;
class Shader;
class Material;

struct UniformAttenuation
{
    GLuint Constant;
    GLuint Linear;
    GLuint Exp;
};
struct UniformPointLight
{
    GLuint Color;
    GLuint Position;

    GLuint AmbientIntensity;
    GLuint DiffuseIntensity;
    GLuint SpecularIntensity;
    GLuint AmbienColor;
    GLuint DiffuseColor;
    GLuint SpecularColor;
    UniformAttenuation Atten;
};

class MaterialUniform
{
public:
    MaterialUniform();
    ~MaterialUniform();

    void SetAmbientColor(Shader *shader, const glm::vec3 &color);
    void SetDiffuseColor(Shader *shader, const glm::vec3 &color);
    void SetSpecularColor(Shader *shader, const glm::vec3 &color);
    void SetShininess(Shader *shader, float shininess);

    void Init(Shader *shader);
    void Apply(Shader *shader);

    GLuint AmbientColor;  // 环境
    GLuint DiffuseColor;  // 漫反射
    GLuint SpecularColor; // 镜面反射
    GLuint Shininess;     // 镜面反射光泽
};

class TechniqueLight : public Technique
{
public:
    TechniqueLight(string name, string vertexShader, string fragmentShader);
    ~TechniqueLight();
    virtual void SetLights(const vector<Light *> &lights);
    void SetDirectionLight(DirectionLight *light);
    void InitPointLightUniform(int num);
    void SetPointLights(vector<PointLight *> lights);
    void SetPointLight(int index, PointLight *light);
    virtual void SetMaterial(const Material *material);

private:
    // 材质
    MaterialUniform MaterialUniform;

    // 光源类型
    GLuint LightTypeUniform;

    // 光源颜色
    GLuint LightColorUniform;
    GLuint LightAmbientUniform;
    GLuint LightDiffuseUniform;
    GLuint LightSpecularUniform;

    // 点光源
    vector<UniformPointLight> PointLightUniforms;
    GLuint PointLightCountUniform;
};

#endif // __TECHNIQUE_LIGHT_H__