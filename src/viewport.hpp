#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "cl_context.hpp"
#include "camera.hpp"
#include <CL/cl.h>

class Viewport
{
public:
    Viewport(const Camera& camera, size_t width, size_t height, size_t rectCount);
    void Draw() const;
    void Update(ClContext &context, size_t i);

private:
    const Camera& camera_;
    size_t width_;
    size_t height_;
    size_t rectCount_;
    size_t currRect_;
    cl_float4* pixels_;

};

#endif // VIEWPORT_HPP
