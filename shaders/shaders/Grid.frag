#version 460

#include <shaders/GridParameters.h>
#include <shaders/GridCalculations.h>

layout (location = 0) in vec2 uv;
layout (location = 1) in vec2 camPos;

layout (location = 2) out vec4 out_Color;

void main()
{
    out_Color = gridColor(uv, cameraPos);
}