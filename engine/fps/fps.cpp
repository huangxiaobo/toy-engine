#include "fps.h"

/*
 * 帧率计数器（FPSCounter）
 *
 * 采用"窗口平均"算法：不逐帧计算瞬时帧率，而是累积一段时间内的帧数，
 * 按实际流逝时间求平均。避免瞬时抖动，同时只在固定间隔（kUpdateIntervalSec）更新，
 * 减少无意义的数值刷新。
 */
FPSCounter::FPSCounter() {
    m_lastTime = std::chrono::steady_clock::now();
}

/*
 * 记录一帧（每帧调用一次）
 *
 * 帧计数器累加，当距上次结算超过刷新间隔时：
 *   FPS = 间隔内帧数 / 间隔时间（秒）
 * 随后清零帧计数并重置时间基准，进入下一个统计窗口。
 */
void FPSCounter::Add() {
    m_fps_now++;

    // 每达到刷新间隔就根据实际流逝时间计算一次帧率
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - m_lastTime).count();
    if (elapsed >= kUpdateIntervalSec) {
        m_fps = static_cast<float>(m_fps_now / elapsed);
        m_fps_now = 0;
        m_lastTime = now;
    }
}

// 获取最近一次结算的帧率（未到结算间隔时返回上一次的值）
float FPSCounter::GetFPS() const {
    return m_fps;
}