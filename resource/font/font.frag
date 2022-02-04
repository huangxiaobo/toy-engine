#version 410

uniform sampler2D texture_material1;

in vec2 Texcoord0;
in vec3 Color0;
out vec4 outputColor;

void main() {
    outputColor = texture(texture_material1, Texcoord0);
}