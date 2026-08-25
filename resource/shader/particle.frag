#version 330 core

in vec3 vColor;
in float vLifeRatio;

uniform sampler2D particleTexture;

out vec4 color;

void main() {
    // 计算点精灵的纹理坐标
    vec2 texCoord = gl_PointCoord;
    
    // 采样纹理
    vec4 texColor = texture(particleTexture, texCoord);
    
    // 根据生命比例调整透明度
    float alpha = texColor.a * (1.0 - vLifeRatio);
    
    // 输出颜色
    color = vec4(vColor * texColor.rgb, alpha);
}
