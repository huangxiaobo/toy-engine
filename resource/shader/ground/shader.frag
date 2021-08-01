#version 330
uniform vec4 gLightPos;
uniform vec3 gViewPos;

in vec3 WorldPos0;
in vec3 Normal0;

out vec4 color;

void main() {

    vec4 DiffuseColor = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);
    float gSpecularPower = 100;
    float gMatSpecularIntensity = 100;


    vec3 Normal = normalize(Normal0);

    // 计算光源到顶点的距离
    vec3 L = gLightPos.xyz - WorldPos0.xyz;

    // g
    vec3 LightDirection = normalize(gLightPos.xyz - WorldPos0.xyz);

    // 计算散射 漫反射
    vec3 diffuse = max(dot(Normal, L), 0.0) * vec3(0.06, 0.04, 0.11);

    float DiffuseFactor = dot(Normal, LightDirection);
    if (DiffuseFactor > 0) {
        vec3 VertexToEye =normalize(gViewPos.xyz - WorldPos0.xyz);
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        float SpecularFactor = dot(VertexToEye, LightReflect);
        if (SpecularFactor > 0) {
            SpecularFactor = pow(SpecularFactor, gSpecularPower);
            SpecularColor = vec4(vec3(0.2, 0.0, 0.0) * gMatSpecularIntensity * SpecularFactor, 1.0f);
        } else {
            SpecularColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    } else {
        SpecularColor = vec4(0.0, 0.0, 1.0, 1.0);
    }
    color = vec4(diffuse + SpecularColor.xyz, 1.0);
}