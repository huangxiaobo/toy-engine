#version 330
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 gWVP;
uniform mat4 gWorld;


layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 2) in vec3 normal;


out vec3 WorldPos0;
out vec3 Normal0;

void main() {
    gl_Position = projection * view * model * vec4(position, 1);


    // 将顶点(x, y, z) 转化成齐次坐标系(homogeneous coords) (x, y, z, w)
    vec4 position_h = vec4(position, 1.0);

    // 模型矩阵
    mat4 mv_matrix = view * model;
    mat3 normalmatrix = mat3(transpose(inverse(model)));

    // 计算顶点在世界坐标系的位置
    WorldPos0 = (model * position_h).xyz;
    // 将法线向量转化到直接坐标系
    Normal0 = normalize(normalmatrix * normal);
}