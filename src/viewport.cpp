#include "viewport.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

Viewport::Viewport(const Camera& camera, size_t width, size_t height, size_t rectCount)
    : camera_(camera), width_(width), height_(height), rectCount_(rectCount), currRect_(0), frameCount_(0)
{
    pixels_ = new cl_float4[width_ * height_];
}

void Viewport::Draw() const
{
    glDrawPixels(width_, height_, GL_RGBA, GL_FLOAT, pixels_);
}

void Viewport::Update(ClContext &context, size_t i, boost::condition_variable& cond)
{
    //std::cout << "Thread: " << i << " " << currRect_ << std::endl;
	context.setArgument(4, sizeof(float3), &camera_.getPos());
	context.setArgument(5, sizeof(float3), &camera_.getFrontVector());
	context.setArgument(6, sizeof(float3), &camera_.getUpVector());

	context.setArgument(10, sizeof(cl_uint), &frameCount_);
	context.setArgument(11, sizeof(cl_uint), &i);
    context.executeKernel(pixels_, width_ * height_ / rectCount_, width_ * height_ / rectCount_ * currRect_);
    
    ++currRect_;
    if (currRect_ >= rectCount_)
    {
        ++frameCount_;
        if (camera_.isChanged())
            frameCount_ = 0;
        currRect_ = 0;
        cond.notify_all();
    }
}
