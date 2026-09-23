#include "light.h"
#include "../utils/utils.h"
#include <glm/gtc/quaternion.hpp>

// ---- 基类 Light ----

/*
 * 默认构造函数
 *
 * 创建未指定类型的灯光：自动生成 UUID，类型置为 LightTypeNone（无类型）。
 * UUID 用于唯一标识光源实例（编辑器中配置 id 覆盖，作为资源稳定标识）。
 */
Light::Light() {
    m_uuid = Utils::GenerateUUID();
    m_light_type = LightTypeNone;
}

/*
 * 带名称和类型的构造函数
 *
 * 参数：
 *   name      - 灯光名称（如 "light-1"），用于编辑器资源列表显示
 *   light_type- 灯光类型（方向光/点光/聚光灯）
 *
 * 子类（如 PointLight）通过该构造函数完成类型初始化，并自动生成独立 UUID。
 */
Light::Light(std::string name, const LightType &light_type)
    : m_name(name), m_light_type(light_type) {
    m_uuid = Utils::GenerateUUID();
}

Light::~Light() {

}

/*
 * 获取灯光类型的可读字符串
 *
 * 用于编辑器属性面板中显示灯光类型，及调试输出。
 * 将枚举值映射为人类可读的中英文名称（常量定义于 light.h）。
 */
const std::string Light::GetLightTypeName() const {
    switch (m_light_type) {
        case LightTypeDirection:
            return LightTypeNameDirection;
        case LightTypePoint:
            return LightTypeNamePoint;
        case LightTypeSpot:
            return LightTypeNameSpot;
        default:
            return LightTypeNameNone;
    }
    return std::string();
}

std::string Light::GetName() const {
    return m_name;
}

std::string Light::GetUUID() const {
    return m_uuid;
}

// ---- 点光源 PointLight ----

/*
 * 点光源构造函数（初始化列表顺序必须与头文件成员声明顺序一致）
 *
 * 所有光照参数默认置 0：
 *   - Position        - 世界空间位置
 *   - Color           - 灯光基础颜色
 *   - Ambient/Diffuse/Specular - 环境光/漫反射/镜面反射三通道分量（含颜色+强度）
 *   - Attenuation     - 衰减系数（常量/线性/指数），定义于 light.h 的 Attenuation 结构体
 */
PointLight::PointLight(const std::string &name) : Light(name, LightTypePoint),
                                           Position(0, 0, 0),
                                           Color(0, 0, 0),
                                           AmbientColor(0, 0, 0),
                                           DiffuseColor(0, 0, 0),
                                           SpecularColor(0, 0, 0),
                                           AmbientIntensity(0),
                                           DiffuseIntensity(0),
                                           SpecularIntensity(0) {
}

PointLight::~PointLight() {
}

/*
 * 点光源可动画属性：位置/颜色/强度
 */
bool PointLight::CanAnimate(AnimProperty prop) const {
    return prop == AnimProperty::Position
        || prop == AnimProperty::LightColor
        || prop == AnimProperty::LightIntensity;
}

/*
 * 写回点光源动画值：Position→位置，LightColor→Color，LightIntensity→漫反射强度
 *
 * 强度动画只取 vec3.x 分量（颜色通道用满三个分量，强度是标量）。
 */
void PointLight::SetAnimValue(AnimProperty prop, const glm::vec3 &value) {
    switch (prop) {
        case AnimProperty::Position:
            Position = value;
            break;
        case AnimProperty::LightColor:
            Color = value;
            break;
        case AnimProperty::LightIntensity:
            DiffuseIntensity = value.x;
            break;
        default:
            break;
    }
}



/*
 * 设置灯光颜色
 *
 * SetColor 与 SetSpecularColor 语义相近，均直接覆盖对应通道，
 * 区别在于 Color 用于可视化光源模型（着色器 uniform "color"），
 * 而 DiffuseColor/SpecularColor 用于光照计算。
 */
void PointLight::SetColor(glm::vec3 color) {
    Color = color;
}

/*
 * 设置光照衰减参数
 *
 * 注意：虽然参数名为 attenuation，但当前实现将值存入 AmbientColor。
 * 调用方若想设置衰减，应直接修改 Attenuation 结构体字段（详见 renderer.cpp 中 Attenuation.Constant 等）。
 */
void PointLight::SetAttenuation(glm::vec3 attenuation) {
    AmbientColor = attenuation;
}

/*
 * 设置灯光位置
 *
 * 位置直接写入 Position 成员（编辑器拖拽时由 ImGui 回调直接修改）。
 * 光源 gizmo 的呈现由 DebugDraw 每帧直接读取 Position 生成，无需显式同步。
 */
void PointLight::SetPosition(glm::vec3 position) {
    Position = position;
}

void PointLight::SetAmbientColor(glm::vec3 direction) {
    AmbientColor = direction;
}

void PointLight::SetDiffuseColor(glm::vec3 color) {
    DiffuseColor = color;
}

void PointLight::SetSpecularColor(glm::vec3 color) {
    SpecularColor = color;
}

// ---- 方向光 DirectionLight ----

DirectionLight::DirectionLight(const std::string &name) : Light(name, LightTypeDirection),
                                               Direction(0, -1, 0),
                                               Color(0, 0, 0),
                                               AmbientColor(0, 0, 0),
                                               DiffuseColor(0, 0, 0),
                                               SpecularColor(0, 0, 0),
                                               AmbientIntensity(0),
                                               DiffuseIntensity(0),
                                               SpecularIntensity(0) {
}

DirectionLight::~DirectionLight() {
}

/*
 * 方向光可动画属性：旋转（欧拉角驱动朝向）/颜色/强度
 *
 * 方向光没有位置概念，故不支持 Position 通道。
 */
bool DirectionLight::CanAnimate(AnimProperty prop) const {
    return prop == AnimProperty::Rotation
        || prop == AnimProperty::LightColor
        || prop == AnimProperty::LightIntensity;
}

/*
 * 写回方向光动画值：Rotation→朝向，LightColor→Color，LightIntensity→漫反射强度
 *
 * 旋转通道把欧拉角（度）作用到基准朝向 (0,-1,0)，重新归一化得到光照方向。
 */
void DirectionLight::SetAnimValue(AnimProperty prop, const glm::vec3 &value) {
    switch (prop) {
        case AnimProperty::Rotation: {
            glm::quat q(glm::radians(value));
            Direction = glm::normalize(q * glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        case AnimProperty::LightColor:
            Color = value;
            break;
        case AnimProperty::LightIntensity:
            DiffuseIntensity = value.x;
            break;
        default:
            break;
    }
}

// ---- 聚光灯 SpotLight ----

SpotLight::SpotLight(const std::string &name) : Light(name, LightTypeSpot),
                                        Position(0, 0, 0),
                                        Direction(0, -1, 0),
                                        Color(0, 0, 0),
                                        AmbientColor(0, 0, 0),
                                        DiffuseColor(0, 0, 0),
                                        SpecularColor(0, 0, 0),
                                        AmbientIntensity(0),
                                        DiffuseIntensity(0),
                                        SpecularIntensity(0),
                                        Cutoff(12.5f),
                                        OuterCutoff(17.5f) {
}

SpotLight::~SpotLight() {
}

/*
 * 聚光灯可动画属性：位置/旋转（欧拉角驱动朝向）/颜色/强度
 */
bool SpotLight::CanAnimate(AnimProperty prop) const {
    return prop == AnimProperty::Position
        || prop == AnimProperty::Rotation
        || prop == AnimProperty::LightColor
        || prop == AnimProperty::LightIntensity;
}

/*
 * 写回聚光灯动画值：Position→位置，Rotation→朝向，LightColor→Color，LightIntensity→漫反射强度
 *
 * 旋转通道与方向光一致：欧拉角（度）作用到基准朝向 (0,-1,0)。
 */
void SpotLight::SetAnimValue(AnimProperty prop, const glm::vec3 &value) {
    switch (prop) {
        case AnimProperty::Position:
            Position = value;
            break;
        case AnimProperty::Rotation: {
            glm::quat q(glm::radians(value));
            Direction = glm::normalize(q * glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        case AnimProperty::LightColor:
            Color = value;
            break;
        case AnimProperty::LightIntensity:
            DiffuseIntensity = value.x;
            break;
        default:
            break;
    }
}