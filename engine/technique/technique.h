#ifndef __TECHNIQUE_H__
#define __TECHNIQUE_H__


#include <glm/glm.hpp>
#include <string>
#include <vector>

using namespace std;

class Shader;

class Technique
{
protected:
    string shaderVertex;
    string shaderFragment;
    Shader *shader_program;

    unsigned int ProjectionUniform;
    unsigned int ViewUniform;
    unsigned int ModelUniform;
    unsigned int WvpUniform; // 模型视图投影矩阵
    unsigned int CameraUniform; // 摄像机位置

public:
    Technique(string name, string vertexShader,  string fragmentShader);
    ~Technique();

    virtual void init();

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