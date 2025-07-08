#include "light.h"
#include "../utils/utils.h"
#include "model/model.h"

Light::Light() {
    m_uuid = Utils::GenerateUUID();
    m_light_type = LightTypeNone;
}

Light::Light(std::string name, const LightType &light_type)
    : m_name(name), m_light_type(light_type) {
    m_uuid = Utils::GenerateUUID();
}

Light::~Light() {

}

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



void PointLight::SetColor(glm::vec3 color) {
    Color = color;
}

void PointLight::SetAttenuation(glm::vec3 attenuation) {
    AmbientColor = attenuation;
}

void PointLight::SetPosition(glm::vec3 position) {
    Position = position;
    if (m_model != nullptr) {
        m_model->SetPosition(position);
    }
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
