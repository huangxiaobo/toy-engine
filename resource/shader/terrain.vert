#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoords;

out VsOut {
    vec3 Color0;
    vec2 TexCoords;
    vec3 WorldPos0;
    vec3 Normal0;
} v2f;

void main() {
    gl_Position = projection * view * model * vec4(position, 1);
    
    v2f.Color0 = vertcolor;
    v2f.TexCoords = texCoords;
    
    // 计算世界空间位置
    vec4 position_h = vec4(position, 1.0);
    v2f.WorldPos0 = (model * position_h).xyz;
    
    // 转换法线到世界空间
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    v2f.Normal0 = normalize(normalMatrix * normal);
}
