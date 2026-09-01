#include "particle_emitter.h"
#include <algorithm>

/*
 * 粒子发射器（ParticleEmitter）
 *
 * 采用"固定容量粒子池 + 复用死亡粒子"的设计：
 *   - 构造时一次性 allocate MaxParticles 个粒子（m_particles），生命周期内不增删内存
 *   - Emit() 从池中找一个死亡粒子重新初始化，实现内存零分配
 *   - Update() 每帧推进粒子物理（重力/阻力/位置积分），并按 EmitRate 发射新粒子
 */
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
    , MinColor(1.0f, 0.5f, 0.0f)
    , MaxColor(1.0f, 1.0f, 0.0f)
    , MinColorEnd(1.0f, 0.0f, 0.0f)
    , MaxColorEnd(0.5f, 0.0f, 0.0f)
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
 *   - 速度/颜色/大小/寿命均在 [Min, Max] 区间随机
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
            p.Color = RandomVec3(MinColor, MaxColor);
            p.ColorEnd = RandomVec3(MinColorEnd, MaxColorEnd);
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

// 每个分量独立在 [min, max] 区间随机，生成随机三维向量
glm::vec3 ParticleEmitter::RandomVec3(const glm::vec3& min, const glm::vec3& max) {
    return glm::vec3(
        RandomFloat(min.x, max.x),
        RandomFloat(min.y, max.y),
        RandomFloat(min.z, max.z)
    );
}