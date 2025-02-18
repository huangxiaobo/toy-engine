#ifndef __TECHNIQUE_LIGHT_H__

#include "technique.h"

class DirectionLight;
class TechniqueLight : private Technique
{
public:
    TechniqueLight(QString name, QString vertexShader, QString fragmentShader);
    ~TechniqueLight();

    virtual void init();

    void SetDirectionLight(DirectionLight *light);

private:
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