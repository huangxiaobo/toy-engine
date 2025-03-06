#ifndef __TECHNIQUE_H__
#define __TECHNIQUE_H__


#include <glm/glm.hpp>
#include <string>
#include <vector>

using namespace std;

class Shader;
class Light;
class Material;


enum TechniqueType {
    TechniqueTypeBase,
    TechniqueTypeLight,
    TechniqueTypeModel,
    TechniqueTypeNormal,
    TechniqueTypeShadow,
    TechniqueTypeShadowMap,
    TechniqueTypeShadowMapLight,
    TechniqueTypeShadowMapModel,
    TechniqueTypeShadowMapNormal,
    TechniqueTypeShadowMapShadow,
    TechniqueTypeShadowMapShadowMap,
    TechniqueTypeShadowMapShadowMapLight,
};





class Technique
{
protected:
    Shader *m_shader;

    TechniqueType m_type;

    unsigned int m_uniform_projection;
    unsigned int m_uniform_view;
    unsigned int m_uniform_model;
    unsigned int m_uniform_wvp; // 模型视图投影矩阵
    unsigned int m_uniform_viewpos; // 摄像机位置

public:
    Technique( string name, string vertexShader,  string fragmentShader);
    virtual ~Technique();

    virtual TechniqueType GetType() const { return m_type ;};

    Shader* GetShader() const ;

    void SetWVP(const glm::mat4& wvp);
    void SetCamera(const glm::vec3 &camera);
    void SetProjection(const glm::mat4& projection);
    void SetView(const glm::mat4& view);
    void SetModel(const glm::mat4& model);
    void SetUniform(const char* name, const glm::vec2& value);
    void SetUniform(const char* name, const glm::vec3& value);
    void SetUniform(const char* name, const glm::vec4& value);
    void SetUniform(const char* name, float value);
    void SetUniform(const char* name, int value);
    void SetUniform();


    virtual void SetLights(const vector<Light *>& lights);
    virtual void SetMaterial(const Material *material);

    void Enable();

    void Disable();
};

#endif