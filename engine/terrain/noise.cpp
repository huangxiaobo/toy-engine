#include "noise.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

/*
 * Perlin噪声实现
 *
 * 核心思想：
 *   1. 将输入空间划分为网格
 *   2. 每个网格顶点有一个伪随机梯度向量
 *   3. 对于任意点，计算其到周围网格顶点的距离向量
 *   4. 用距离向量与梯度向量的点积，通过平滑插值得到最终噪声值
 */

Noise::Noise() {
    // 初始化排列表 - 0-255的随机排列
    // 这个表用于将坐标映射到伪随机梯度
    for (int i = 0; i < 256; i++) {
        m_perm[i] = i;
    }
    // Fisher-Yates洗牌算法
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(m_perm[i], m_perm[j]);
    }
    // 复制一份，避免索引时取模
    for (int i = 0; i < 256; i++) {
        m_perm[i + 256] = m_perm[i];
    }
}

Noise::~Noise() {
}

void Noise::SetSeed(int seed) {
    srand(seed);
    // 重新生成排列表
    for (int i = 0; i < 256; i++) {
        m_perm[i] = i;
    }
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(m_perm[i], m_perm[j]);
    }
    for (int i = 0; i < 256; i++) {
        m_perm[i + 256] = m_perm[i];
    }
}

/*
 * Fade函数 - 平滑插值曲线
 * 使用 6t^5 - 15t^4 + 10t^3（改进的Perlin噪声）
 * 相比原始的 3t^2 - 2t^3，二阶导数连续，消除视觉伪影
 */
float Noise::Fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/*
 * 线性插值
 */
float Noise::Lerp(float t, float a, float b) const {
    return a + t * (b - a);
}

/*
 * 梯度函数 - 根据哈希值选择梯度方向
 * 使用8个预定义方向（2D平面上的单位向量）
 */
float Noise::Gradient(int hash, float x, float z) const {
    int h = hash & 7;  // 取低3位，范围0-7
    float u = (h < 4) ? x : z;
    float v = (h < 4) ? z : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

/*
 * 2D Perlin噪声
 *
 * 算法步骤：
 *   1. 确定点所在的网格单元
 *   2. 计算点在单元内的相对位置
 *   3. 用排列表获取四个角的哈希值
 *   4. 计算点到四个角的梯度点积
 *   5. 双线性插值得到最终值
 */
float Noise::Perlin2D(float x, float z) const {
    // 网格单元坐标
    int xi = static_cast<int>(std::floor(x)) & 255;
    int zi = static_cast<int>(std::floor(z)) & 255;

    // 单元内的相对位置 [0, 1]
    float xf = x - std::floor(x);
    float zf = z - std::floor(z);

    // 平滑插值因子
    float u = Fade(xf);
    float v = Fade(zf);

    // 四个角的哈希值
    int aa = m_perm[m_perm[xi] + zi];
    int ab = m_perm[m_perm[xi] + zi + 1];
    int ba = m_perm[m_perm[xi + 1] + zi];
    int bb = m_perm[m_perm[xi + 1] + zi + 1];

    // 四个角的梯度点积
    float g00 = Gradient(aa, xf, zf);
    float g10 = Gradient(ba, xf - 1.0f, zf);
    float g01 = Gradient(ab, xf, zf - 1.0f);
    float g11 = Gradient(bb, xf - 1.0f, zf - 1.0f);

    // 双线性插值
    float x1 = Lerp(u, g00, g10);
    float x2 = Lerp(u, g01, g11);
    return Lerp(v, x1, x2);
}

/*
 * 分形噪声(Fractional Brownian Motion)
 *
 * 原理：叠加多个不同频率和振幅的噪声层
 *   - 第n层：频率 = baseFrequency × lacunarity^n
 *            振幅 = baseAmplitude × persistence^n
 *
 * 典型参数：
 *   octaves=6, lacunarity=2.0, persistence=0.5
 *   → 产生自然的地形起伏，从大山脉到小岩石细节
 */
float Noise::FBM(float x, float z, int octaves,
                 float lacunarity, float persistence) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;  // 用于归一化

    for (int i = 0; i < octaves; i++) {
        // 叠加当前层的噪声
        value += Perlin2D(x * frequency, z * frequency) * amplitude;

        // 累加最大可能值（用于归一化）
        maxValue += amplitude;

        // 下一层：频率翻倍，振幅减半
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    // 归一化到 [-1, 1]
    return value / maxValue;
}
