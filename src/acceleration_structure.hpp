#ifndef ACCELERATION_STRUCTURE_HPP
#define ACCELERATION_STRUCTURE_HPP

#include "kernels/shared_structures.h"
#include <vector>
#include <CL/cl.hpp>

class CLContext;
class AccelerationStructure
{
public:
    AccelerationStructure(CLContext& cl_context)
        : cl_context_(cl_context)
    {}

    virtual void BuildCPU(std::vector<Triangle> & triangles) = 0;
    virtual void IntersectRays(cl::Buffer const& rays_buffer, cl::Buffer const& ray_counter_buffer,
        std::uint32_t max_num_rays, cl::Buffer const& hits_buffer) = 0;

protected:
    CLContext& cl_context_;

};

#endif // ACCELERATION_STRUCTURE_HPP
