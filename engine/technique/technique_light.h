#ifndef __TECHNIQUE_LIGHT_H__
#define __TECHNIQUE_LIGHT_H__
#include <glad/gl.h> //  必须在所有库的顶部

#include "technique.h"

class DirectionLight;
class PointLight;
class SpotLight;
class Shader;
class Material;

struct UniformAttenuation
{
    GLuint m_constant;
    GLuint m_linear;
    GLuint m_exp;
};
struct UniformPointLight
{
    GLuint m_color;
    GLuint m_position;

    GLuint m_ambient_intensity;
    GLuint m_diffuse_intensity;
    GLuint m_specular_intensity;
    GLuint m_ambient_color;
    GLuint m_diffuse_color;
    GLuint m_specular_color;
    UniformAttenuation m_atten;
};

struct UniformDirectionLight
{
    GLuint m_direction;
    GLuint m_color;
    GLuint m_ambient_intensity;
    GLuint m_diffuse_intensity;
    GLuint m_specular_intensity;
    GLuint m_ambient_color;
    GLuint m_diffuse_color;
    GLuint m_specular_color;
};

struct UniformSpotLight
{
    GLuint m_position;
    GLuint m_direction;
    GLuint m_color;
    GLuint m_ambient_intensity;
    GLuint m_diffuse_intensity;
    GLuint m_specular_intensity;
    GLuint m_ambient_color;
    GLuint m_diffuse_color;
    GLuint m_specular_color;
    UniformAttenuation m_atten;
    GLuint m_cutoff;
    GLuint m_outer_cutoff;
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

    GLuint m_ambient_color;  // 环境
    GLuint m_diffuse_color;  // 漫反射
    GLuint m_specular_color; // 镜面反射
    GLuint m_shininess;     // 镜面反射光泽
};

class TechniqueLight : public Technique
{
public:
    TechniqueLight(std::string name, std::string vertexShader, std::string fragmentShader);
    ~TechniqueLight();
    virtual void SetLights(const std::vector<Light *> &lights);
    void SetDirectionLight(DirectionLight *light);
    void InitDirectionLightUniform();
    void InitPointLightUniform(int num);
    void SetPointLights(std::vector<PointLight *> lights);
    void SetPointLight(int index, PointLight *light);
    void SetSpotLight(int index, SpotLight *light);
    void InitSpotLightUniform(int num);
    void SetSpotLights(std::vector<SpotLight *> lights);
    virtual void SetMaterial(const Material *material);

private:
    // 材质
    MaterialUniform m_material_uniform;

    // 方向光
    UniformDirectionLight m_direction_light_uniform;

    // 点光源
    std::vector<UniformPointLight> m_point_light_uniforms;
    GLuint m_point_light_count_uniform;

    // 聚光灯
    std::vector<UniformSpotLight> m_spot_light_uniforms;
    GLuint m_spot_light_count_uniform;
};

#endif // __TECHNIQUE_LIGHT_H__