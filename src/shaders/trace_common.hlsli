/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

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

#include "common.hlsli"

// Acceleration structure data
StructuredBuffer<RTTriangle> g_Triangles : register(t3);
StructuredBuffer<LinearBVHNode> g_Nodes : register(t4);

bool IntersectTriangle(
    float3 origin, float3 direction, float t_min, inout float t_max, RTTriangle tri, out float2 bc)
{
    float3 p0 = tri.position1;
    float3 p1 = tri.position2;
    float3 p2 = tri.position3;
    float3 e1 = p1 - p0;
    float3 e2 = p2 - p0;
    float3 p = cross(direction, e2);
    float det = dot(e1, p);

    if (abs(det) < 1.0e-8f)
    {
        bc = 0.0f.xx;
        return false;
    }

    float inv_det = 1.0f / det;
    float3 s = origin - p0;
    float u = dot(s, p) * inv_det;
    if (u < 0.0f || u > 1.0f)
    {
        bc = 0.0f.xx;
        return false;
    }

    float3 q = cross(s, e1);
    float v = dot(direction, q) * inv_det;
    if (v < 0.0f || u + v > 1.0f)
    {
        bc = 0.0f.xx;
        return false;
    }

    float t = dot(e2, q) * inv_det;
    if (t < t_min || t > t_max)
    {
        bc = 0.0f.xx;
        return false;
    }

    bc = float2(u, v);
    t_max = t;
    return true;
}

bool IntersectBounds(
    float3 bmin, float3 bmax, float3 ray_origin, float3 ray_inv_dir, float t_min, float t_max)
{
    float3 t0 = (bmin - ray_origin) * ray_inv_dir;
    float3 t1 = (bmax - ray_origin) * ray_inv_dir;
    float3 tsmaller = min(t0, t1);
    float3 tbigger = max(t0, t1);
    float near_t = max(max(tsmaller.x, tsmaller.y), max(tsmaller.z, t_min));
    float far_t = min(min(tbigger.x, tbigger.y), min(tbigger.z, t_max));
    return far_t >= near_t;
}

Hit TraceBVH(float3 ray_origin, float3 ray_direction, float t_min, float t_max, uint node_count,
    bool any_hit)
{
    Hit hit;
    hit.bc = 0.0f.xx;
    hit.primitive_id = INVALID_ID;
    hit.instance_id = INVALID_ID;
    hit.t = t_max;
    hit.padding1 = 0u;
    hit.padding2 = 0u;
    hit.padding3 = 0u;

    if (node_count == 0)
    {
        return hit;
    }

    float3 ray_inv_dir = 1.0f.xxx / ray_direction;
    uint3 ray_sign = ray_inv_dir < 0.0f.xxx;
    uint nodes_to_visit[64];
    uint to_visit_offset = 0;
    uint current_node_index = 0;

    [loop] while (true)
    {
        LinearBVHNode node = g_Nodes[current_node_index];
        if (IntersectBounds(node.bmin, node.bmax, ray_origin, ray_inv_dir, t_min, hit.t))
        {
            uint num_primitives = node.num_primitives_axis >> 16;
            if (num_primitives > 0)
            {
                [loop] for (uint i = 0; i < num_primitives; ++i)
                {
                    float2 bc;
                    float t = hit.t;
                    if (IntersectTriangle(
                            ray_origin, ray_direction, t_min, t, g_Triangles[node.offset + i], bc))
                    {
                        hit.bc = bc;
                        hit.primitive_id = g_Triangles[node.offset + i].prim_id;
                        hit.instance_id = g_Triangles[node.offset + i].instance_id;
                        hit.t = t;
                        if (any_hit)
                        {
                            return hit;
                        }
                    }
                }

                if (to_visit_offset == 0)
                {
                    break;
                }
                current_node_index = nodes_to_visit[--to_visit_offset];
            }
            else
            {
                uint axis = node.num_primitives_axis & 0xFFFFu;
                bool sign =
                    axis == 0 ? ray_sign.x != 0 : (axis == 1 ? ray_sign.y != 0 : ray_sign.z != 0);
                if (sign)
                {
                    nodes_to_visit[to_visit_offset++] = current_node_index + 1;
                    current_node_index = node.offset;
                }
                else
                {
                    nodes_to_visit[to_visit_offset++] = node.offset;
                    current_node_index = current_node_index + 1;
                }
            }
        }
        else
        {
            if (to_visit_offset == 0)
            {
                break;
            }
            current_node_index = nodes_to_visit[--to_visit_offset];
        }
    }

    return hit;
}
