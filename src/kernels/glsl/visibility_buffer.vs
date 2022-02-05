#version 430 core

layout (location = 0) uniform mat4 g_ViewProjection;
varying vec2 vTexcoord;

struct Vertex
{
    vec4 position;
    vec4 texcoord;
    vec4 normal;
    vec4 tangent_s;
};

struct Triangle
{
    Vertex v1, v2, v3;
    unsigned int mtlIndex;
    unsigned int padding[3];
};

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
