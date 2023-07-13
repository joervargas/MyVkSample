#pragma once

#include <assert.h>
#include <stdio.h>


class FramesPerSecondCounter
{
public:

    explicit FramesPerSecondCounter(float avgInterval = 0.5f)
        : m_avgInterval(avgInterval)
    {
        assert(avgInterval > 0.0f);
    }

    bool tick(float deltaSeconds, bool frameRendered = true)
    {
        if(frameRendered) { m_numFrames++; }

        m_accumulatedTime += deltaSeconds;
        if(m_accumulatedTime > m_avgInterval)
        {
            m_currentFPS = static_cast<float>(m_numFrames / m_accumulatedTime);
            if(printFPS) 
            {
                printf("FPS: %.1f\n", m_currentFPS);
            }
            m_numFrames = 0;
            m_accumulatedTime = 0;
            return true;
        }

        return false;
    }

    inline float getFPS() const { return m_currentFPS; }

    bool printFPS = true;

private:

    const float m_avgInterval = 0.5f;
    unsigned int m_numFrames = 0;
    double m_accumulatedTime = 0.0;
    float m_currentFPS = 0.0f;

};