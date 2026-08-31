#ifndef __MTL_PARSER_H__
#define __MTL_PARSER_H__

/**
 * 自定义 MTL（Material Template Library）文件解析器。
 *
 * MTL 是 OBJ 模型格式配套的材质库文件，由 OBJ 文件中的 mtllib 指令引用。
 * 本解析器自己实现（不依赖 Assimp 的材质解析），按 Wavefront OBJ 规范
 * 解析 MTL 中与当前引擎 Material 对应的基础字段：
 *
 *   newmtl <name>  声明一个新的材质（后续指令都属于该材质）
 *   Ka r g b       环境光颜色（Ambient），对应 Material::AmbientColor
 *   Kd r g b       漫反射颜色（Diffuse），对应 Material::DiffuseColor
 *   Ks r g b       镜面反射颜色（Specular），对应 Material::SpecularColor
 *   Ns <float>     高光指数（0~1000），对应 Material::Shininess
 *
 * 颜色分量取值范围为 0.0~1.0，与引擎 Material 中的 glm::vec3 保持一致。
 */

#include <string>
#include <vector>

#include <glm/glm.hpp>

class Material;

class MtlParser {
public:
    MtlParser();
    ~MtlParser();

    /**
     * 解析 MTL 文件并返回文件内定义的所有材质。
     *
     * @param filePath MTL 文件的路径（相对或绝对路径均可）
     * @return 解析成功的 Material 列表；解析失败（文件打不开）时返回空列表。
     *         调用方负责管理返回的 Material 内存。
     */
    std::vector<Material *> ParseFromFile(const std::string &filePath);

    /**
     * 从 MTL 文件中解析并返回指定名称的材质。
     *
     * 这是 world.yaml 中引用材质的便捷接口：
     *   先 ParseFromFile 得到全部材质，再按 name 匹配 newmtl 声明的材质名。
     *
     * @param filePath MTL 文件的路径
     * @param materialName 要查找的材质名（对应 newmtl <name>）
     * @return 匹配的 Material 指针；未找到或解析失败时返回 nullptr（不新增内存）。
     */
    Material *ParseSingle(const std::string &filePath, const std::string &materialName);
};

#endif // __MTL_PARSER_H__
