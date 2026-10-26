#pragma once
#include <cstdint>

// Monotonic deadlines keep emulated time independent of GUI timer frequency.
class FramePacer
{
public:
    void reset() { m_next = 0; }
    bool shouldRun(int64_t now, int64_t period, int catchUpFrames = 0)
    {
        if (now < m_next) return false;
        // Presentation drops late frames. Emulation can recover a small,
        // bounded backlog to avoid drifting behind the audio clock.
        if (catchUpFrames > 0) {
            if (now - m_next > period * (catchUpFrames + 1))
                m_next = now - period * catchUpFrames;
            m_next += period;
        } else {
            m_next = now - m_next >= period ? now + period : m_next + period;
        }
        return true;
    }
private:
    int64_t m_next = 0;
};
