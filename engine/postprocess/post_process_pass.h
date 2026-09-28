#pragma once

#include <memory>

class Technique;

// 全屏三角形 VAO 与后处理参数；场景颜色纹理由调用方每帧传入
class PostProcessPass {
public:
    PostProcessPass() = default;
    ~PostProcessPass();

    void Init();
    void Render(unsigned int scene_color_texture);

    void SetToneMappingEnabled(bool enabled) { m_tone_mapping_enabled = enabled; }
    bool IsToneMappingEnabled() const { return m_tone_mapping_enabled; }

    void SetExposure(float exposure) { m_exposure = exposure; }
    float GetExposure() const { return m_exposure; }

    void SetSaturation(float sat) { m_saturation = sat; }
    float GetSaturation() const { return m_saturation; }

    void SetContrast(float contrast) { m_contrast = contrast; }
    float GetContrast() const { return m_contrast; }

private:
    std::unique_ptr<Technique> m_post_tech;
    // Core Profile 未绑定 VAO 时绘制会报错，而全屏三角形坐标由 gl_VertexID 生成，
    // 故需一个空 VAO 占位
    unsigned int m_post_vao = 0;
    bool m_tone_mapping_enabled = true;
    float m_exposure = 1.0f;
    float m_saturation = 1.0f;
    float m_contrast = 1.0f;
};
