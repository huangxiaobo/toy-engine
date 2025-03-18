#version 330 core

uniform mat4 projection; // 投影矩阵
uniform mat4 view;
uniform mat4 model;
uniform mat4 gWVP;
uniform mat4 gWorld;


layout (location = 0) in vec3 VertexPosition; //in关键字表示该变量是输入的数据,layout (location = 0)设定了输入变量的位置值(Location)
layout (location = 1) in vec3 VertexColor;//in关键字表示该变量是输入的数据,layout (location = 1)设定了输入变量的位置值(VertColor)
layout (location = 2) in vec3 VertexNormal;//in关键字表示该变量是输入的数据,layout (location = 2)设定了输入变量的位置值(Normal)

out VsOut {
    vec3 Color0;
    vec3 WorldPos0;
    vec3 Normal0;
} v2f;


void main() {
    // 将顶点(x, y, z) 转化成齐次坐标系(homogeneous coords) (x, y, z, w)
    vec4 position_h = vec4(VertexPosition, 1.0);

    // 模型矩阵
    mat4 mv_matrix = view * model;
    mat3 normalmatrix = mat3(transpose(inverse(model)));

    // 计算顶点在世界坐标系的位置
    v2f.WorldPos0 = (model * position_h).xyz;
    // 将法线向量转化到直接坐标系
    v2f.Normal0 = normalize(normalmatrix * VertexNormal);
    // 顶点颜色
    v2f.Color0 = VertexColor;

    gl_Position = projection * view * model * vec4(VertexPosition, 1);
}