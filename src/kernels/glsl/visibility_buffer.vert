/*****************************************************************************
 MIT License

 Copyright(c) 2023 Alexander Veselov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this softwareand associated documentation files(the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions :

 The above copyright noticeand this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *****************************************************************************/

#include "src/kernels/common/shared_structures.h"

layout (location = 0) uniform mat4 g_ViewProjection;
layout (location = 0) out flat uint out_geometry_info;

layout (binding = 1, std430) buffer TriangleBuffer
{
    Triangle triangles[];
};

void main()
{
    uint triangle_idx = gl_VertexID / 3;
    Triangle triangle = triangles[triangle_idx];

    int vertex_idx = (gl_VertexID % 3);
    vec3 pos[3] = { triangle.v1.position.xyz,
                    triangle.v2.position.xyz,
                    triangle.v3.position.xyz };

    gl_Position = g_ViewProjection * vec4(pos[vertex_idx], 1.0);
    out_geometry_info = triangle_idx;
}
