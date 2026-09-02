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

    // 设置灯光名称（供 world.yaml 配置 name 字段使用）
    void SetName(const std::string &name) { m_name = name; }

    // 设置灯光 ID（供 world.yaml 配置 id 字段使用，用于稳定标识光源资源）
    void SetUUID(const std::string &uuid) { m_uuid = uuid; }

    void SetModel(Model *model) { m_model = model; };
    Model *GetModel() const { return m_model; }

    // 是否启用该灯光（false 时不参与光照计算，相当于关闭灯光）
    bool IsEnabled() const { return m_enabled; }

    // 开关灯光：true 启用参与光照，false 关闭（默认启用）
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    virtual ~Light();

private:
    LightType m_light_type;
    std::string m_name;
    std::string m_uuid;
    bool m_enabled = true;

protected:
    Model *m_model;
};

class DirectionLight : public Light {
public:
    /*
     * 方向光（平行光）构造函数
     *
     * 方向光模拟无限远处的光源（如太阳），所有光线方向平行，
     * 无位置概念，无衰减。整个场景接收相同方向和强度的光照。
     */
    DirectionLight(const std::string &name);

    ~DirectionLight() override;

    glm::vec3 Direction;   // 光照方向（从光源指向场景）
    glm::vec3 Color;       // 光源基础颜色

    glm::vec3 AmbientColor;   // 环境光颜色
    glm::vec3 DiffuseColor;   // 漫反射颜色
    glm::vec3 SpecularColor;  // 镜面反射颜色

    float AmbientIntensity;   // 环境光强度
    float DiffuseIntensity;   // 漫反射强度
    float SpecularIntensity;  // 镜面反射强度
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

class SpotLight : public Light {
public:
    /*
     * 聚光灯构造函数
     *
     * 聚光灯模拟手电筒、舞台灯等锥形光源：有位置、方向、锥角。
     * 在内锥（Cutoff）内全亮，内锥到外锥（OuterCutoff）之间线性衰减。
     */
    SpotLight(const std::string &name);

    ~SpotLight() override;

    glm::vec3 Position;    // 光源位置
    glm::vec3 Direction;   // 光照方向（从光源指向照射目标）
    glm::vec3 Color;       // 光源基础颜色

    glm::vec3 AmbientColor;   // 环境光颜色
    glm::vec3 DiffuseColor;   // 漫反射颜色
    glm::vec3 SpecularColor;  // 镜面反射颜色

    float AmbientIntensity;   // 环境光强度
    float DiffuseIntensity;   // 漫反射强度
    float SpecularIntensity;  // 镜面反射强度

    struct {
        float Constant;       // 衰减常数项
        float Linear;         // 衰减线性项
        float Exp;            // 衰减指数项
    } Attenuation{};

    float Cutoff;          // 内锥角（度），全亮范围
    float OuterCutoff;     // 外锥角（度），衰减到 0 的范围
};

#endif // __LIGHT_H__
