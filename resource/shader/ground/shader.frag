#version 330

in vec4 fcolour;
out vec4 color;
void main() {
    color = vec4(0.2, 0.2, 0.2, 1.0) * fcolour;
}