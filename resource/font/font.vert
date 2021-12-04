#version 410
uniform mat4 projection;
uniform mat4 model;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;

out vec2 fragTexCoord;

void main() {
    fragTexCoord = texcoord;
    gl_Position = projection * model * vec4(position, 0, 1);
}