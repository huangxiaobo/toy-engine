#ifndef __NOISE_H__
#define __NOISE_H__

/*
 * Perlin噪声生成器
 *
 * 用于程序化地形高度生成，支持多层叠加（分形噪声/FBM）
 *
 * 算法原理：
 *   1. 基础Perlin噪声：在网格点生成随机梯度向量，通过插值生成平滑噪声
 *   2. 分形噪声(FBM)：叠加多个不同频率和振幅的噪声层
 *      - 频率(Frequency)：控制噪声的细节密度
 *      - 振幅(Amplitude)：控制噪声的高度影响
 *      - 每层频率翻倍、振幅减半 → 产生自然的地形细节层次
 *
 * 使用方法：
 *   Noise noise;
 *   noise.SetSeed(12345);  // 可选，设置随机种子
 *   float height = noise.FBM(x, z, octaves, lacunarity, persistence);
 */

class Noise {
public:
    Noise();
    ~Noise();

    // 设置随机种子，确保相同种子产生相同地形
    void SetSeed(int seed);

    // 2D Perlin噪声，输入坐标(x,z)，输出范围[-1, 1]
    float Perlin2D(float x, float z) const;

    // 分形噪声(FBM) - 叠加多层噪声生成自然地形
    // x, z: 世界坐标
    // octaves: 叠加层数（6-8层可产生丰富细节）
    // lacunarity: 频率倍增因子（通常为2.0）
    // persistence: 振幅衰减因子（通常为0.5）
    float FBM(float x, float z, int octaves = 6,
              float lacunarity = 2.0f, float persistence = 0.5f) const;

private:
    // 内部插值函数
    float Fade(float t) const;
    float Lerp(float t, float a, float b) const;
    float Gradient(int hash, float x, float z) const;

    // 排列表（Permutation table）- Perlin噪声的核心数据结构
    // 256个随机值，通过模运算循环使用
    int m_perm[512];
};

#endif // __NOISE_H__
