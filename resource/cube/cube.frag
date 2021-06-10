#version 330
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

in vec3 FragPos;
in vec3 Normal;
out vec4 color;
void main() {
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    color = vec4(diffuse + ambient, 1.0);
}