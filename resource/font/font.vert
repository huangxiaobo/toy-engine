#version 410
uniform mat4 projection;
uniform mat4 model;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texcoord;


out vec2 Texcoord0;
out vec3 Color0;

void main() {
    Texcoord0 = texcoord;
    Color0 = vertcolor;
    gl_Position = projection * model * vec4(position, 1);
}