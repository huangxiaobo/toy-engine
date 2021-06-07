#version 330
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

in vec3 position;
in vec3 normal;

out vec3 FragPos;
out vec3 Normal;
void main() {
    gl_Position = projection * view * model * vec4(position, 1);
    FragPos = vec3(model * vec4(position, 1.0));
    Normal = normal;
}