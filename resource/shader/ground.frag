#version 330 core

in VsOut {
    vec3 Color0;
    vec2 TexCoords;
} v2f;

uniform sampler2D groundTexture;

out vec4 color;

void main() {
    vec4 texColor = texture(groundTexture, v2f.TexCoords);
    color = vec4(v2f.Color0, 1.0f) * texColor;
}
