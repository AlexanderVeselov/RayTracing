#ifndef ACCELERATION_STRUCTURE_HPP
#define ACCELERATION_STRUCTURE_HPP

#include "Utils/shared_structs.hpp"
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
    virtual void IntersectRays(cl::CommandQueue const& queue,
        cl::Buffer const& rays_buffer, cl::Buffer const& hits_buffer) = 0;

protected:
    CLContext& cl_context_;

};

#endif // ACCELERATION_STRUCTURE_HPP
