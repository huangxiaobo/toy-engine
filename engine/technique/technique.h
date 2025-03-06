#ifndef __TECHNIQUE_H__
#define __TECHNIQUE_H__


#include <glm/glm.hpp>
#include <string>
#include <vector>

using namespace std;

class Shader;

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
    string m_shader_vert;
    string m_shader_frag;
    Shader *m_shader;

    TechniqueType m_type;

    unsigned int ProjectionUniform;
    unsigned int ViewUniform;
    unsigned int ModelUniform;
    unsigned int WvpUniform; // 模型视图投影矩阵
    unsigned int CameraUniform; // 摄像机位置

public:
    Technique( string name, string vertexShader,  string fragmentShader);
    virtual ~Technique();

    virtual void init();

    TechniqueType GetType() const { return m_type ;};

    Shader* GetShader() const ;

    void SetWVP(const glm::mat4& wvp);
    void SetCamera(const glm::vec3 &camera);
    void SetProjection(const glm::mat4& projection);
    void SetView(const glm::mat4& view);
    void SetModel(const glm::mat4& model);
    void draw(long long elapsed);
    void setUniform(const char* name, const glm::vec2& value);
    void setUniform(const char* name, const glm::vec3& value);
    void setUniform(const char* name, const glm::vec4& value);
    void setUniform(const char* name, float value);
    void setUniform(const char* name, int value);
    void setUniform();

    void Enable();

    void Disable();
};

#endif