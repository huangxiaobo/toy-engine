/*
 * lit.vert —— 标准管线 ③ 顶点颜色 x 材质 x 光照（Blinn-Phong + 阴影 + 法线贴图）
 *
 * 功能最完整的标准渲染管线，作为场景中受光模型的默认选择。
 * 输出顶点所需的全部世界空间量：位置 / 法线 / 颜色 / UV / TBN（切线空间基）。
 *
 * 顶点属性布局（与 engine/mesh/mesh.h 的 Vertex 布局一致）：
 *   location 0 = position    顶点位置
 *   location 1 = vertcolor   顶点颜色（参与 albedo 合成）
 *   location 2 = normal      顶点法线（变换到世界空间做光照）
 *   location 3 = texcoords   纹理坐标（采样漫反射/法线贴图）
 *   location 4 = tangent     切线（法线贴图 TBN 构造用）
 *   location 5 = bitangent   副切线（法线贴图 TBN 构造用）
 *
 * 矩阵 uniform：
 *   projection / view / model —— 模型 -> 世界 -> 观察 -> 裁剪
 *   lightSpace                 —— 光源正交投影 x 光源视图（阴影采样坐标，见 Technique::SetLightSpaceMatrix）
 */
#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpace;   // 光源空间矩阵：把世界坐标变换到光源视角裁剪空间（阴影判定用）

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertcolor;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texcoords;
layout (location = 4) in vec3 tangent;
layout (location = 5) in vec3 bitangent;

out VS_OUT {
    vec3  Color0;          // 顶点颜色
    vec2  TexCoords;       // 纹理坐标
    vec3  WorldPos0;       // 世界空间顶点位置（光照/视向量计算用）
    vec3  Normal0;         // 世界空间法线
    vec3  Tangent0;        // 世界空间切线（TBN 行向量 1）
    vec3  Bitangent0;      // 世界空间副切线（TBN 行向量 2）
    vec4  FragPosLightSpace; // 光源裁剪空间顶点位置（阴影贴图采样用）
} v2f;

void main() {
    // 透视裁剪坐标
    gl_Position = projection * view * model * vec4(position, 1.0);

    v2f.Color0    = vertcolor;
    v2f.TexCoords = texcoords;

    // 世界空间位置
    vec4 worldPos = model * vec4(position, 1.0);
    v2f.WorldPos0 = worldPos.xyz;

    // 法线矩阵：非均匀缩放下法线需用模型矩阵的逆转置变换，并归一化
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    v2f.Normal0    = normalize(normalMatrix * normal);
    // 切线/副切线同样经法线矩阵变换（法线贴图在切空间内偏移）。
    // 经 TBN 构造后可还原出带贴图扰动后的世界空间法线，详见片元着色器。
    v2f.Tangent0   = normalize(normalMatrix * tangent);
    v2f.Bitangent0 = normalize(normalMatrix * bitangent);

    // 光源空间坐标必须基于已乘 model 的世界坐标，与深度 Pass 的变换一致
    //（gDepthMVP = lightSpace * model），否则阴影坐标会与深度贴图错位。
    v2f.FragPosLightSpace = lightSpace * worldPos;
}