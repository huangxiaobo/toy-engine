#version 330
in vec3 xyz;
out vec4 color;
void main() {
    color = vec4(normalize(xyz * 100), 1.0);
}