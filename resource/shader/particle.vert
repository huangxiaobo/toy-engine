#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in float size;
layout (location = 3) in float life;
layout (location = 4) in float maxLife;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 vColor;
out float vLifeRatio;

void main() {
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;
    
    // 计算生命比例
    vLifeRatio = life / maxLife;
    vColor = color;
    
    // 根据生命比例调整大小
    gl_PointSize = size * (1.0 + (1.0 - vLifeRatio) * 0.5);
}
