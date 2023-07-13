#pragma once

#include <deque>
#include <limits>

#include "ProfilerWrapper.h"
#include "VulkanCanvas.h"

#include <glm/glm.hpp>
using glm::vec3;
using glm::vec4;

class LinearGraph
{
public:

    explicit LinearGraph(size_t maxGraphPoints = 256) :
        m_maxPoints(maxGraphPoints)
    {}

    void addPoint(float value)
    {
        m_graph.push_back(value);
        if(m_graph.size() > m_maxPoints)
        {
            m_graph.pop_front();
        }
    }

    void renderGraph(VulkanCanvas& canvas, const glm::vec4& color = vec4(1.0)) const
    {
        EASY_FUNCTION();

        float minfps = std::numeric_limits<float>::max();
        float maxfps = std::numeric_limits<float>::min();

        for(float f : m_graph)
        {
            if(f < minfps) { minfps = f; }
            if(f > maxfps) { maxfps = f; }
        }

        const float range = maxfps - minfps;
        float x = 0.0;
        vec3 p1 = vec3(0, 0, 0);
        for(float f : m_graph)
        {
            const float val = (f - minfps) / range;
            const vec3 p2 = vec3(x, val * 0.15f, 0);
            x += 1.0f / m_maxPoints;
            canvas.line(p1, p2, color);
            p1 = p2;
        }
    }


private:

    std::deque<float> m_graph;
    const size_t m_maxPoints;

};