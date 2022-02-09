#include "src/kernels/shared_structures.h"

layout (location = 0) uniform mat4 g_ViewProjection;
varying vec2 vTexcoord;

layout (binding = 1, std430) buffer TriangleBuffer
{
    Triangle triangles[];
};

void main()
{
    vTexcoord = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);

    Triangle triangle = triangles[gl_VertexID / 3];

    int vertex_idx = (gl_VertexID % 3);
    vec3 pos[3] = { triangle.v1.position.xyz,
                    triangle.v2.position.xyz,
                    triangle.v3.position.xyz };

    gl_Position = g_ViewProjection * vec4(pos[vertex_idx], 1.0);
}