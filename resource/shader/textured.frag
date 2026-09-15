/*
 * textured.frag —— 标准管线 ② 顶点颜色 x 材质贴图（无光照）
 *
 * 颜色合成：最终颜色 = 顶点颜色 x 漫反射贴图颜色。
 *   - 网格绑定了漫反射贴图（gHasTexture = 1）：albedo = 顶点色 x 贴图
 *   - 无贴图（gHasTexture = 0）：退化为纯顶点颜色（等价 unlit 管线）
 *
 * gHasTexture 由 Mesh::Draw 按网格实际绑定的纹理上传（见 mesh.cpp），
 * 避免在无纹理时采样到残留/脏纹理单元导致的错误着色。
 */
#version 330 core

// 漫反射贴图：绑定到纹理单元 0（Mesh::Draw 约定，见 mesh.cpp）
uniform sampler2D gTexture;
// 网格是否绑定了漫反射贴图（0 = 未绑定，退化为顶点色）
uniform int gHasTexture = 0;

in VS_OUT {
    vec3  Color0;    // 顶点颜色
    vec2  TexCoords; // 纹理坐标
} v2f;

out vec4 color;

void main() {
    // 顶点色 x 贴图（无贴图时 gTexture 项为 1.0，纯顶点色输出）
    vec3 albedo = v2f.Color0;
    if (gHasTexture == 1) {
        albedo *= texture(gTexture, v2f.TexCoords).rgb;
    }
    color = vec4(albedo, 1.0);
}