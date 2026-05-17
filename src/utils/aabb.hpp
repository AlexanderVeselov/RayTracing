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

#pragma once

#include <cassert>
#include <limits>

#include <glm/glm.hpp>

struct Aabb
{
    Aabb()
    {
        float minNum = std::numeric_limits<float>::lowest();
        float maxNum = (std::numeric_limits<float>::max)();
        min = glm::vec3(maxNum);
        max = glm::vec3(minNum);
    }

    explicit Aabb(glm::vec3 p) : min{p}, max{p} {}

    Aabb(glm::vec3 p1, glm::vec3 p2) : min{(glm::min)(p1, p2)}, max{(glm::max)(p1, p2)} {}

    const glm::vec3& operator[](int i) const
    {
        assert(i == 0 || i == 1);
        return (i == 0) ? min : max;
    }

    glm::vec3& operator[](int i)
    {
        assert(i == 0 || i == 1);
        return (i == 0) ? min : max;
    }

    glm::vec3 Corner(int corner) const
    {
        return glm::vec3((*this)[corner & 1].x, (*this)[(corner & 2) ? 1 : 0].y, (*this)[(corner & 4) ? 1 : 0].z);
    }

    glm::vec3 Diagonal() const { return max - min; }

    float SurfaceArea() const
    {
        glm::vec3 d = Diagonal();
        return 2.0f * (d.x * d.y + d.x * d.z + d.y * d.z);
    }

    float Volume() const
    {
        glm::vec3 d = Diagonal();
        return d.x * d.y * d.z;
    }

    unsigned int MaximumExtent() const
    {
        glm::vec3 d = Diagonal();
        if (d.x > d.y && d.x > d.z)
            return 0;
        if (d.y > d.z)
            return 1;
        return 2;
    }

    glm::vec3 Offset(const glm::vec3& p) const
    {
        glm::vec3 o = p - min;
        if (max.x > min.x)
            o.x /= max.x - min.x;
        if (max.y > min.y)
            o.y /= max.y - min.y;
        if (max.z > min.z)
            o.z /= max.z - min.z;
        return o;
    }

    glm::vec3 min;
    glm::vec3 max;
};

inline Aabb Union(const Aabb& b, const glm::vec3& p)
{
    Aabb ret;
    ret.min = (glm::min)(b.min, p);
    ret.max = (glm::max)(b.max, p);
    return ret;
}

inline Aabb Union(const Aabb& b1, const Aabb& b2)
{
    Aabb ret;
    ret.min = (glm::min)(b1.min, b2.min);
    ret.max = (glm::max)(b1.max, b2.max);
    return ret;
}
