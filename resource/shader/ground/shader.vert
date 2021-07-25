#version 330
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec4 lightpos;

in vec3 position;
in vec3 normal;

out vec4 fcolour;

void main() {
    gl_Position = projection * view * model * vec4(position, 1);

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
    vec3 L = lightpos.xyz - P.xyz;

    // 计算散射
    vec3 diffuse = max(dot(N, L), 0.0) * vec3(0.06, 0.04, 0.11);
    fcolour = vec4(diffuse, 1.0);
}