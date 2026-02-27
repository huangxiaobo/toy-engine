#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoords;

out VsOut {
    vec3 Color0;
    vec2 TexCoords;
} v2f;

void main() {
    v2f.Color0 = vertcolor;
    v2f.TexCoords = texCoords;
    gl_Position = projection * view * model * vec4(position, 1);
}
