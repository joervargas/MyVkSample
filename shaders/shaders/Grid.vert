#version 460 core

#include <shaders/GridParameters.h>
#include <shaders/BufferDeclarations.h>

layout (location = 0) out vec2 uv;
layout (location = 1) out vec2 out_cameraPos;

void main()
{
    mat4 view_proj = proj * view;

    int idx = indices[gl_VertexID];
    vec3 position = pos[idx] * gridSize;

    position.x += cameraPos.x;
    position.y += cameraPos.y;

    gl_Position = view_proj * vec4(position, 1.0);

    out_cameraPos = cameraPos.xz;
    uv = position.xz;
}