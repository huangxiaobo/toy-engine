#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 gWVP;
uniform mat4 gWorld;


layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 2) in vec3 normal;


out VsOut {
    vec3 Color0;
} v2f;


void main() {
    v2f.Color0 = vertcolor;

    gl_Position = projection * view * model * vec4(position, 1);
}