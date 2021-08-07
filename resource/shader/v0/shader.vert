#version 330
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
struct Attenuation
{
    float Constant;
    float Linear;
    float Exp;
};

struct Light {
    vec3    Color;
    vec3    Position;

    float   AmbientIntensity;
    float   DiffuseIntensity;
    vec3    DiffuseColor;
    vec3    SpecularColor;
    Attenuation Atten;
};

uniform Light gLight[1];

// 材质结构体
struct Material{
    vec3 AmbientColor;//环境
    vec3 DiffuseColor;//漫反射
    vec3 SpecularColor;//镜面反射
    float Shininess;//镜面反射光泽
};

uniform Material gMaterial;

in vec3 position;
in vec3 normal;

out vec4 fcolour;
void main() {
    // 将顶点(x, y, z) 转化成齐次坐标系(homogeneous coords) (x, y, z, w)
    vec4 position_h = vec4(position, 1.0);

    // 模型矩阵
    mat4 mv_matrix = view * model;
    mat3 normalmatrix = mat3(transpose(inverse(mv_matrix)));

    // 计算顶点在世界坐标系的位置
    vec4 P = mv_matrix * position_h;
    // 将法线向量转化到直接坐标系
    vec3 N = normalize(normalmatrix * normal);
    // 计算光源到顶点的距离
    vec3 L = gLight[0].Position.xyz - P.xyz;

    // 计算散射
    vec3 diffuse = max(dot(N, L), 0.0) * gMaterial.DiffuseColor * gLight[0].Color;
    fcolour = vec4(diffuse, 1.0);

    gl_Position = projection * view * model * vec4(position, 1);
}