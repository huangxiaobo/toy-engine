#include "fps.h"
#include <thread>
#include <chrono>

FPSCounter::FPSCounter() : m_fps_now(0), m_fps(0), m_stop(false)
{
    // 启动定时器线程
    m_timer_thread = std::thread(&FPSCounter::UpdateFPS, this);
}

FPSCounter::~FPSCounter()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cv.notify_one();
    if (m_timer_thread.joinable())
    {
        m_timer_thread.join();
    }
}

void FPSCounter::Add()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_fps_now++;
}

float FPSCounter::GetFPS()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_fps;
}

void FPSCounter::UpdateFPS()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (!m_stop)
    {
        // 等待 1 秒
        m_cv.wait_for(lock, std::chrono::seconds(1));
        // 更新帧率
        m_fps = m_fps_now;
        // 重置帧数计数器
        m_fps_now = 0;
    }
}
