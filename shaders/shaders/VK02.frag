#version 460

layout( location = 0 ) in vec3 fragColor;
layout( location = 1 ) in vec3 barycoords;
layout( location = 2 ) in vec2 uv;
layout( location = 0 ) out vec4 outColor;

layout( binding = 3 ) uniform sampler2D texSampler;


float edgeFactor(float thickness)
{
    vec3 a3 = smoothstep(vec3(0.0), fwidth(barycoords) * thickness, barycoords);
    return min(min(a3.x, a3.y), a3.z);
}

void main()
{
    outColor = vec4(
        mix(
            vec3(0.0), // start range of mix interpolation
            texture(texSampler, uv).xyz, // end range of mix interpolation
            edgeFactor(1.0) // value to interpolate by
        ), 
        1.0
    );
}