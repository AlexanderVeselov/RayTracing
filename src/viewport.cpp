#include "viewport.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

Viewport::Viewport(const Camera& camera, size_t width, size_t height, size_t rectCount)
    : camera_(camera), width_(width), height_(height), rectCount_(rectCount), currRect_(0)
{
    pixels_ = new cl_float4[width_ * height_];
}

void Viewport::Draw() const
{
    glDrawPixels(width_, height_, GL_RGBA, GL_FLOAT, pixels_);
}

void Viewport::Update(ClContext &context, size_t i)
{
    std::cout << "Thread: " << i << " " << currRect_ << std::endl;
	context.setArgument(4, sizeof(float3), &camera_.getPos());
	context.setArgument(5, sizeof(float3), &camera_.getFrontVector());
	context.setArgument(6, sizeof(float3), &camera_.getUpVector());
    context.executeKernel(pixels_, width_ * height_ / rectCount_, width_ * height_ / rectCount_ * currRect_);
    
    ++currRect_;
    if (currRect_ >= rectCount_)
    {
        currRect_ = 0;
    }
}
