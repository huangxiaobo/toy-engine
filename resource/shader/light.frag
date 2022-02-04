#version 330

in vec3 Color0;

out vec4 color;
void main() {
    color = vec4(Color0, 1.0);
}