#ifndef __LIGHT_H__
#define __LIGHT_H__

#include <glm/glm.hpp>
#include <memory>
#include <string>

class Model;

enum LightType {
    LightTypeNone, // 未定义
    LightTypePoint, // 点光源
    LightTypeDirection, // 方向光源
    LightTypeSpot // 聚光灯
};

const std::string LightTypeNameNone = "未定义";
const std::string LightTypeNamePoint = "点光源";
const std::string LightTypeNameDirection = "方向光源";
const std::string LightTypeNameSpot = "聚光灯";


class Light {
public:
    Light();

    Light(std::string name, const LightType &light_type);

    virtual const LightType &GetLightType() const { return m_light_type; };

    virtual const std::string GetLightTypeName() const;

    std::string GetName() const;

    std::string GetUUID() const;

    void SetModel(Model *model) { m_model = model; };
    Model *GetModel() const { return m_model; }

    virtual ~Light();

private:
    LightType m_light_type;
    std::string m_name;
    std::string m_uuid;

protected:
    Model *m_model;
};

class DirectionLight : public Light {
public:
    glm::vec3 Direction;
    glm::vec3 Color;

    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;

    float AmbientIntensity;
    float DiffuseIntensity;
    float SpecularIntensity;
};

// Atten参数参考表
// Distance	Constant    Linear    Quadratic
// 200	    1.0	        0.022      0.0019
// 325	    1.0	        0.014      0.0007
// 600	    1.0	        0.007      0.0002
class PointLight : public Light {
public:
    glm::vec3 Position;
    glm::vec3 Color;

    glm::vec3 AmbientColor;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;

    float AmbientIntensity;
    float DiffuseIntensity;
    float SpecularIntensity;

    struct {
        float Constant;
        float Linear;
        float Exp;
    } Attenuation{};

public:
    PointLight(const std::string &name);

    ~PointLight() override;

    void SetColor(glm::vec3 color);

    void SetAttenuation(glm::vec3 attenuation);

    void SetPosition(glm::vec3 position);

    void SetAmbientColor(glm::vec3 direction);

    void SetDiffuseColor(glm::vec3 color);

    void SetSpecularColor(glm::vec3 color);
};

#endif // __LIGHT_H__
