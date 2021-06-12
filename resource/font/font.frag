#version 330

uniform sampler2D tex;
uniform vec4 color;

in vec2 fragTexCoord;
out vec4 outputColor;

void main() {
    outputColor = color * texture(tex, fragTexCoord);
}