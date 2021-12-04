#version 330
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
in vec3 position;
out vec3 xyz;
void main() {
    gl_Position = projection * view * model * vec4(position, 1);
    xyz = position;
}