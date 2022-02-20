#version 330
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;

out VsOut {
    vec3 Color0;
} v2f;

void main() {
    v2f.Color0 = vertcolor;
    gl_Position = projection * view * model * vec4(position, 1);
}