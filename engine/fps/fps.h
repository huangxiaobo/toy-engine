#ifndef __FPS_H__
#define __FPS_H__

#include <chrono>

/*
 * 帧率统计器
 *
 * 使用单调时钟在调用方(主线程)内联统计，避免了原先起一个后台线程 + 互斥锁的实现，
 * 消除了每帧对锁的获取开销以及额外的线程生命周期管理。
 */
class FPSCounter {
public:
    FPSCounter();

    // 每帧调用一次，累计帧数并周期性刷新 FPS 值
    void Add();

    // 获取最近一次统计出的帧率(无锁)
    float GetFPS() const;

private:
    // 距上次刷新以来累计的帧数
    unsigned int m_fps_now = 0;
    // 刷新 FPS 的时间起点
    std::chrono::steady_clock::time_point m_lastTime;
    // 最近一次统计出的帧率
    float m_fps = 0.0f;
    // FPS 刷新间隔(秒)
    static constexpr double kUpdateIntervalSec = 1.0;
};

#endif // __FPS_H__
