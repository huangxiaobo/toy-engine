#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <string>
#include <vector>

#include "camera/camera.h" // 摄像机投影枚举（CameraConfig::Projection 使用）

// 这两组字段会在 world.yaml 缺失时直接决定窗口与投影，必须带默认值：
// LoadFromYaml 遇到 BadFile 会吞掉异常并返回默认实例，读取的就是这里写的数。
class WindowConfig {
public:
    int m_window_width = 1280;
    int m_window_height = 720;
};

class ClipConfig {
public:
    float m_clip_near = 0.1f;
    float m_clip_far = 1000.0f;
    float m_clip_fov = 45.0f;
    float m_clip_aspect = 16.0f / 9.0f;
};

class CameraConfig {
public:
    std::string m_name;  // 摄像机名称
    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_up;
    ProjectionType m_projection = ProjectionType::Perspective; // 投影模式（perspective 透视 / orthographic 正交，默认透视）
};

class PointLightConfig {
public:
    std::string m_name;
    std::string m_id;
    bool m_enabled = true;
    glm::vec3 m_position;
    glm::vec3 m_color;

    glm::vec3 m_ambient_color;
    glm::vec3 m_diffuse_color;
    glm::vec3 m_specular_color;

    // 三通道光照强度（world.yaml ambient/diffuse/specular 的 intensity 字段），默认 1.0
    float m_ambient_intensity = 1.0f;
    float m_diffuse_intensity = 1.0f;
    float m_specular_intensity = 1.0f;

    struct {
        float m_constant;
        float m_linear;
        float m_exp;
    } Attenuation;
};

class DirectionLightConfig {
public:
    std::string m_name;
    std::string m_id;
    bool m_enabled = true;
    glm::vec3 m_direction;
    glm::vec3 m_color;

    glm::vec3 m_ambient_color;
    glm::vec3 m_diffuse_color;
    glm::vec3 m_specular_color;

    // 三通道光照强度（world.yaml ambient/diffuse/specular 的 intensity 字段），默认 1.0
    float m_ambient_intensity = 1.0f;
    float m_diffuse_intensity = 1.0f;
    float m_specular_intensity = 1.0f;
};

class SpotLightConfig {
public:
    std::string m_name;
    std::string m_id;
    bool m_enabled = true;
    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::vec3 m_color;

    glm::vec3 m_ambient_color;
    glm::vec3 m_diffuse_color;
    glm::vec3 m_specular_color;

    // 三通道光照强度（world.yaml ambient/diffuse/specular 的 intensity 字段），默认 1.0
    float m_ambient_intensity = 1.0f;
    float m_diffuse_intensity = 1.0f;
    float m_specular_intensity = 1.0f;

    struct {
        float m_constant;
        float m_linear;
        float m_exp;
    } Attenuation;

    float m_cutoff;
    float m_outer_cutoff;
};

/*
 * 材质配置（MaterialConfig）
 *
 * 对应 world.yaml 中 model 的 material 段。为支持「用 MTL 实现材质」，
 * 材质不再内联在 yaml 中，而是通过 MTL 文件路径 + 材质名引用：
 *
 *   material:
 *     file: "./resource/model/sphere/sphere.mtl"  # MTL 文件路径
 *     name: "sphere_material"                     # newmtl 声明的材质名
 *
 * 渲染器使用 MtlParser 从 MTL 文件加载出 Material 对象。
 */
class MaterialConfig {
public:
    std::string m_file; // MTL 文件路径
    std::string m_name; // 材质名称（对应 newmtl <name>）
};

class ParticleConfig {
public:
    std::string m_name;
    std::string m_id;
    glm::vec3 m_position;
    
    float m_emit_rate;
    int m_max_particles;
    
    float m_min_life, m_max_life;
    float m_min_size, m_max_size;
    glm::vec3 m_min_velocity, m_max_velocity;
    float m_min_size_end, m_max_size_end;
    
    glm::vec3 m_gravity;
    float m_drag;
};

class MeshConfig {
public:
    std::string m_name;
    std::string m_file;
};

class SkyDomeConfig {
public:
    float m_radius = 500.0f;
    int m_sectors = 32;
    int m_stacks = 16;
    glm::vec3 m_horizon_color = glm::vec3(0.6f, 0.7f, 0.9f);
    glm::vec3 m_zenith_color = glm::vec3(0.1f, 0.2f, 0.5f);
    // 地面雾色（下半球），默认取地平线色 0.6 倍（暗化大气色）
    glm::vec3 m_ground_color = glm::vec3(0.36f, 0.42f, 0.54f);
};

/*
 * 地形配置（TerrainConfig）
 *
 * 对应 world.yaml 中的 terrain 段，用于配置程序化网格地形（无 LOD）。
 * 这些参数会被解析后传给 TerrainManager（engine/terrain/terrain_manager.h）。
 *
 * 各字段含义：
 *   - planeSize:     地形平面世界尺寸（长 = 宽，以原点为中心）
 *   - resolution:    网格分辨率（每边格子数，顶点数 = resolution + 1）
 *   - heightScale:   地形最大高度（噪声高度缩放因子，0 = 平坦平面）
 *   - noiseSeed:     噪声生成器的随机种子，相同种子产生相同地形
 */
class TerrainConfigCfg {
public:
    float m_plane_size = 100.0f;
    int m_resolution = 128;
    float m_height_scale = 20.0f;
    unsigned int m_noise_seed = 12345;
};

class ModelConfig {
public:
    std::string m_name;
    std::string m_effect;
    std::string m_shader_vert_file;
    std::string m_shader_frag_file;

    glm::vec3 m_position;
    // 三轴欧拉角（度）：x/y/z 分量分别对应绕 X/Y/Z 轴，旋转顺序 Y → X → Z
    glm::vec3 m_rotation;
    glm::vec3 m_scale;

    MeshConfig m_mesh;

    MaterialConfig m_material;
};

/*
 * 动画配置（AnimationConfig）
 *
 * 对应 world.yaml 的 animations 段，绑定一个已存在的模型或灯光做动画。
 * 各通道（位移/缩放/旋转/颜色/强度）各自独立可选，未配置的通道保持对象原值：
 *
 *   - translate: 正弦振荡 position = center + amplitude * sin(2π·frequency·t)
 *   - scale:     正弦振荡 scale      = base + amplitude * sin(2π·frequency·t)
 *   - rotate:    非 spin 时正弦摆动；spin 模式下匀速旋转（speed 各轴度/秒）
 *   - color:     灯光颜色正弦振荡（R,G,B 振幅）
 *   - intensity: 灯光强度正弦振荡（振幅取 vec3.x，仅灯光可动画）
 *
 * ModelName/LightName 为引用的目标名（world.yaml models[].m_name / lights[].m_name），
 * 由渲染器查找绑定；两字段最多填一个。
 */
class AnimationConfig {
public:
    std::string m_name;
    std::string m_model_name;   // 绑定的模型名（与 light_name 二选一）
    std::string m_light_name;   // 绑定的灯光名（与 model_name 二选一）
    bool m_enabled = true;

    // ---- 位移通道（可选）----
    bool m_has_translate = false;
    glm::vec3 translate_center{};     // 振荡中心（世界位置）
    glm::vec3 translate_amplitude{};  // 单侧振幅
    float m_translate_frequency = 1.0f; // 频率（Hz）

    // ---- 圆周轨道通道（可选）----
    // 绕 Y 轴画圈：x/z 分量 90° 相位差（cos/sin），y 固定为轨道中心高度
    // 点光源无朝向，"绕 Y 轴旋转"即此语义（ambient 同级字段，与 translate 二选一）
    bool m_has_orbit = false;
    glm::vec3 orbit_center{};         // 轨道中心（世界位置）
    glm::vec3 orbit_radius{};         // 轨道半径（x/z 分量生效）
    float m_orbit_frequency = 1.0f;     // 角速度（圈/秒）

    // ---- 缩放通道（可选）----
    bool m_has_scale = false;
    glm::vec3 scale_base{};           // 缩放中心
    glm::vec3 scale_amplitude{};      // 单侧振幅
    float m_scale_frequency = 1.0f;     // 频率（Hz）

    // ---- 旋转通道（可选）----
    bool m_has_rotate = false;
    bool m_rotate_spin = false;         // true = 匀速旋转（用 speed），false = 正弦摆动
    glm::vec3 rotate_base{};          // 旋转基准（欧拉角，度）
    glm::vec3 rotate_amplitude{};     // 摆动振幅（欧拉角，度）
    float m_rotate_frequency = 1.0f;    // 摆动频率（Hz）
    glm::vec3 rotate_speed{};         // 匀速旋转角速度（度/秒）

    // ---- 灯光颜色通道（可选）----
    bool m_has_color = false;
    glm::vec3 color_center{};         // 颜色振荡中心（R,G,B）
    glm::vec3 color_amplitude{};      // 各通道单侧振幅
    float m_color_frequency = 1.0f;     // 频率（Hz）

    // ---- 灯光强度通道（可选）----
    bool m_has_intensity = false;
    glm::vec3 intensity_center{};     // 强度中心（取 x 分量）
    glm::vec3 intensity_amplitude{};  // 单侧振幅（取 x 分量）
    float m_intensity_frequency = 1.0f; // 频率（Hz）
};

class Config {
public:
    Config();

    ~Config();

    // 解析 world.yaml。文件缺失时返回一份默认构造实例（不返回 nullptr），
    // 解析中途出错抛异常，局部对象随栈展开自动释放。
    static std::unique_ptr<Config> LoadFromYaml(const std::string &filename);

    WindowConfig m_window;
    glm::vec4 m_clear_color;
    ClipConfig m_clip;
    std::vector<CameraConfig> m_cameras;  // 支持多个摄像机
    std::vector<PointLightConfig> m_point_lights;
    std::vector<DirectionLightConfig> m_direction_lights;
    std::vector<SpotLightConfig> m_spot_lights;
    std::vector<ParticleConfig> m_particles;
    SkyDomeConfig m_sky_dome;
    std::vector<ModelConfig> m_models;
    std::vector<AnimationConfig> m_animations;

    // 地形配置（程序化LOD地形）
    TerrainConfigCfg m_terrain;
};

#endif // __CONFIG_H__
