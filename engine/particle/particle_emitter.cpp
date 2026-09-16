#include "particle_emitter.h"
#include <algorithm>

/*
 * 粒子发射器（ParticleEmitter）
 *
 * 采用"固定容量粒子池 + 复用死亡粒子"的设计：
 *   - 构造时一次性 allocate MaxParticles 个粒子（m_particles），生命周期内不增删内存
 *   - Emit() 从池中找一个死亡粒子重新初始化，实现内存零分配
 *   - Update() 每帧推进粒子物理（重力/阻力/位置积分），并按 EmitRate 发射新粒子
 *
 * 颜色采用"烟花配色"方案（Emit 时绕过 Min/Max 色域，直接使用调色板）：
 *   - 出生色（p.Color）   = 调色板基色向白色混合 —— 模拟烟花引燃瞬间的炽热白核
 *   - 结束色（p.ColorEnd） = 同一基色按比例变暗 —— 模拟火花冷却熄灭，保持同色相渐变
 *   - 每个粒子的色相独立随机，呈现出五彩斑斓的烟花效果
 */

// 烟花配色表：经典烟花的高饱和颜色（RGB 线性空间）
// 金色 / 明黄 / 橙红 / 火红 / 粉红 / 品红 / 紫罗兰 / 蓝色 / 青色 / 翠绿
static const glm::vec3 kFireworkPalette[] = {
    glm::vec3(1.00f, 0.84f, 0.00f),  // 金色
    glm::vec3(1.00f, 1.00f, 0.20f),  // 明黄
    glm::vec3(1.00f, 0.55f, 0.00f),  // 橙红
    glm::vec3(1.00f, 0.10f, 0.05f),  // 火红
    glm::vec3(1.00f, 0.25f, 0.50f),  // 粉红
    glm::vec3(1.00f, 0.00f, 1.00f),  // 品红
    glm::vec3(0.70f, 0.00f, 1.00f),  // 紫罗兰
    glm::vec3(0.00f, 0.40f, 1.00f),  // 蓝色
    glm::vec3(0.00f, 1.00f, 0.90f),  // 青色
    glm::vec3(0.10f, 1.00f, 0.20f),  // 翠绿
};

static const size_t kFireworkPaletteSize = sizeof(kFireworkPalette) / sizeof(kFireworkPalette[0]);

ParticleEmitter::ParticleEmitter()
    : Position(0.0f)
    , EmitRate(100.0f)
    , MaxParticles(1000)
    , MinLife(1.0f)
    , MaxLife(3.0f)
    , MinSize(0.1f)
    , MaxSize(0.5f)
    , MinVelocity(-1.0f, 2.0f, -1.0f)
    , MaxVelocity(1.0f, 5.0f, 1.0f)
    , MinSizeEnd(0.0f)
    , MaxSizeEnd(0.1f)
    , Gravity(0.0f, -9.8f, 0.0f)
    , Drag(0.98f)
    , m_emitAccumulator(0.0f)
    , m_gen(m_rd()) {

    // 预分配粒子池并全部标记为死亡（Life=0），供后续 Emit 复用
    m_particles.resize(MaxParticles);
    for (auto& p : m_particles) {
        p.Life = 0.0f;
    }
}

ParticleEmitter::~ParticleEmitter() {
}

/*
 * 调整粒子池容量
 *
 * 扩容后新粒子需要显式清零所有字段，避免未初始化数据（脏数据）被当作存活粒子
 * 或产生未定义值（NaN 等）参与渲染。
 */
void ParticleEmitter::SetMaxParticles(int maxParticles) {
    int oldSize = m_particles.size();
    MaxParticles = maxParticles;
    m_particles.resize(MaxParticles);

    // 初始化新添加的粒子
    for (int i = oldSize; i < MaxParticles; i++) {
        m_particles[i].Life = 0.0f;
        m_particles[i].Age = 0.0f;
        m_particles[i].Position = glm::vec3(0.0f);
        m_particles[i].Velocity = glm::vec3(0.0f);
        m_particles[i].Color = glm::vec3(0.0f);
        m_particles[i].ColorEnd = glm::vec3(0.0f);
        m_particles[i].Size = 0.0f;
        m_particles[i].SizeEnd = 0.0f;
        m_particles[i].MaxLife = 0.0f;
    }
}

/*
 * 每帧更新所有存活粒子 + 按速率发射新粒子
 *
 * 步骤：
 *   1. 生命周期推进：Life 递减、Age 递增，死亡粒子（IsAlive=false）跳过物理
 *   2. 物理积分：速度 += 重力 * dt；速度 *= 阻力；位置 += 速度 * dt
 *      （显式欧拉积分，dt 为固定帧步长）
 *   3. 发射累积：m_emitAccumulator 累加 EmitRate*dt，累积满 1 就发射一个粒子。
 *      这样即使帧率波动，发射速率也能保持统计平均一致。
 */
void ParticleEmitter::Update(float deltaTime) {
    // 先更新所有粒子生命
    for (auto& p : m_particles) {
        if (!p.IsAlive()) continue;

        p.Life -= deltaTime;
        p.Age += deltaTime;

        if (p.IsAlive()) {
            // 应用重力
            p.Velocity += Gravity * deltaTime;

            // 应用阻力
            p.Velocity *= Drag;

            // 更新位置
            p.Position += p.Velocity * deltaTime;
        }
    }

    // 然后发射新粒子（现在有死粒子可复用）
    m_emitAccumulator += EmitRate * deltaTime;
    while (m_emitAccumulator >= 1.0f) {
        Emit();
        m_emitAccumulator -= 1.0f;
    }
}

/*
 * 发射一个粒子（复用粒子池中的死亡粒子）
 *
 * 线性扫描找到第一个死亡粒子（Life <= 0），用随机属性重新初始化：
 *   - 初始位置 = 发射器位置
 *   - 速度/大小/寿命均在 [Min, Max] 区间随机
 *   - 颜色采用"烟花配色"：出生色为白热核心，结束色为同色相变暗（见文件头注释）
 *
 * 粒子池满时（全部存活）静默失败，不产生新粒子。
 */
void ParticleEmitter::Emit() {
    // 查找一个死粒子
    for (auto& p : m_particles) {
        if (!p.IsAlive()) {
            // 初始化粒子
            p.Position = Position;
            p.Velocity = RandomVec3(MinVelocity, MaxVelocity);

            // ---- 烟花配色 ----
            // 1. 从调色板随机抽取一个高饱和基色（含 ±10% 明度抖动，避免千篇一律）
            const glm::vec3 base = RandomFireworkColor();

            // 2. 出生色 = 基色向白色混合 55%：模拟烟花引燃瞬间的炽热白光，
            //    再由片段着色器按生命比例平滑过渡到结束色
            p.Color = glm::mix(base, glm::vec3(1.0f), 0.55f);

            // 3. 结束色 = 基色压暗到 45%：模拟火花冷却熄灭。
            //    保持色相不变只降明度，避免 RGB 区间随机带来的浑浊混色
            p.ColorEnd = base * 0.45f;

            p.Size = RandomFloat(MinSize, MaxSize);
            p.SizeEnd = RandomFloat(MinSizeEnd, MaxSizeEnd);
            p.MaxLife = RandomFloat(MinLife, MaxLife);
            p.Life = p.MaxLife;
            p.Age = 0.0f;
            return;
        }
    }
}

/*
 * 统计当前存活粒子数
 *
 * O(n) 遍历粒子池；绘制前用于判断是否可跳过本次 draw（无存活粒子时直接 return）。
 */
int ParticleEmitter::GetAliveCount() const {
    int count = 0;
    for (const auto& p : m_particles) {
        if (p.IsAlive()) count++;
    }
    return count;
}

// 在 [min, max] 区间生成均匀随机浮点数
float ParticleEmitter::RandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_gen);
}

// 在 [min, max] 区间生成均匀随机整数（闭区间）
int ParticleEmitter::RandomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_gen);
}

// 每个分量独立在 [min, max] 区间随机，生成随机三维向量
glm::vec3 ParticleEmitter::RandomVec3(const glm::vec3& min, const glm::vec3& max) {
    return glm::vec3(
        RandomFloat(min.x, max.x),
        RandomFloat(min.y, max.y),
        RandomFloat(min.z, max.z)
    );
}

/*
 * 从烟花调色板随机抽取一个高饱和基色
 *
 * 对常规 RGB 区间随机而言，三个分量独立取值极难同时命中高饱和组合，
 * 产出的大多是发灰、浑浊的中间色。烟花配色表预先定义好 10 种高饱和色相，
 * 只对明度做 ±10% 抖动，保证每个粒子都鲜艳且色相纯正。
 */
glm::vec3 ParticleEmitter::RandomFireworkColor() {
    const int index = RandomInt(0, static_cast<int>(kFireworkPaletteSize) - 1);
    const float jitter = RandomFloat(0.9f, 1.1f);
    return glm::clamp(kFireworkPalette[index] * jitter, 0.0f, 1.0f);
}