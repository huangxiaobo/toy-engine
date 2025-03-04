#version 330 core

uniform mat4 mat_model;
uniform mat4 mat_view;
uniform mat4 mat_projection;


layout (location = 0) in vec3 position;

void main() {
    gl_Position = mat_projection * mat_view * mat_model * vec4(position, 1);
}