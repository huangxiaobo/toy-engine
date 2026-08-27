#include "fps.h"

FPSCounter::FPSCounter() {
    m_lastTime = std::chrono::steady_clock::now();
}

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

float FPSCounter::GetFPS() const {
    return m_fps;
}
