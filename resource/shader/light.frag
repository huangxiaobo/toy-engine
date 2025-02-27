#version 330 core

in VsOut {
    vec3 Color0;
} v2f;

out vec4 FragColor;
void main() {
    FragColor = vec4(v2f.Color0, 1.0f);
}