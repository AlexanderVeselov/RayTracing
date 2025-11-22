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

#include "mathlib.hpp"
#include "kernels/common/shared_structures.h"

void Bounds3::Project(float3 axis, float &mins, float &maxs) const
{
    mins = CL_FLT_MAX;
    maxs = -CL_FLT_MAX;

    float3 points[8] = {
        min,
        float3(min.x, min.y, max.z),
        float3(min.x, max.y, min.z),
        float3(min.x, max.y, max.z),
        float3(max.x, min.y, min.z),
        float3(max.x, min.y, max.z),
        float3(max.x, max.y, min.z),
        max
    };

    for (size_t i = 0; i < 8; ++i)
    {
        float val = Dot(points[i], axis);
        mins = std::min(mins, val);
        maxs = std::max(maxs, val);
    }
}
