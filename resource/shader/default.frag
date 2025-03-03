#version 330 core

in VsOut {
    vec3 Color0;
} v2f;

out vec4 color;

void main() {
    color = vec4(v2f.Color0, 1.0f);
}