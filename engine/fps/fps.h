#ifndef __FPS_H__
#define __FPS_H__

#include <mutex>
#include <condition_variable>
#include <thread>

class FPSCounter
{
public:
    FPSCounter();
    ~FPSCounter();
    void Add();
    float GetFPS();

private:
    void UpdateFPS();

private:
    int m_frame_count;
    float m_fps;
    bool m_stop;
    std::thread m_timer_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

#endif // __FPS_H__