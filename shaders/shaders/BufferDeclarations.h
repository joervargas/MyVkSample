layout(std140, binding = 0) uniform PerFrameData
{
    mat4 view;
    mat4 proj;
    vec4 camerPos;
};

struct Vertex
{
    float p[3];
    float n[3];
    float tc[2];
};

layout(std430, binding = 1) restrict readonly buffer Vertices
{
    Vertex in_vertices[];
};

layout(std430, binding = 2) restrict readonly buffer Matrices
{
    mat4 in_model_mats[];
};