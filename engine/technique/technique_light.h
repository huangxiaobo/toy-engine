#ifndef __TECHNIQUE_LIGHT_H__
#include <glad/gl.h> //  必须在所有库的顶部


#include "technique.h"

class DirectionLight;

class MaterialUniform
{
public:
    MaterialUniform();
    ~MaterialUniform();

    void SetAmbientColor(const glm::vec3 &color);
    void SetDiffuseColor(const glm::vec3 &color);
    void SetSpecularColor(const glm::vec3 &color);
    void SetShininess(float shininess);

    void Init(GLuint program);
    void Apply(GLuint program);

    GLuint AmbientColor;  // 环境
    GLuint DiffuseColor;  // 漫反射
    GLuint SpecularColor; // 镜面反射
    GLuint Shininess;     // 镜面反射光泽
};

class TechniqueLight : private Technique
{
public:
    TechniqueLight(string name, string vertexShader, string fragmentShader);
    ~TechniqueLight();

    virtual void init();

    void SetDirectionLight(DirectionLight *light);

private:

    // 材质
    MaterialUniform material;

    // 光源类型
    GLuint LightTypeUniform;

    // 光源颜色
    GLuint LightColorUniform;
    GLuint LightAmbientUniform;
    GLuint LightDiffuseUniform;
    GLuint LightSpecularUniform;

    // 点光源
    GLuint LightPositionUniform;
    // 方向光
    GLuint LightDirectionUniform;
    // 聚光灯
    GLuint LightIntensityUniform;
    GLuint LightAttenuationUniform;
};

#endif // __TECHNIQUE_LIGHT_H__