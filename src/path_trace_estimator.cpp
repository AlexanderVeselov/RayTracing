#include "path_trace_estimator.hpp"
#include "utils/cl_exception.hpp"
#include "Scene/scene.hpp"
#include "Scene/camera.hpp"
#include <GL/glew.h>

PathTraceEstimator::PathTraceEstimator(std::uint32_t width, std::uint32_t height,
    CLContext& cl_context, cl_GLuint gl_interop_image)
    : width_(width)
    , height_(height)
    , cl_context_(cl_context)
    , gl_interop_image_(gl_interop_image)
{
    // Create buffers and images
    cl_int status;

    radiance_buffer_ = std::make_unique<cl::Buffer>(cl_context.GetContext(), CL_MEM_READ_WRITE,
        width_ * height_ * sizeof(cl_float4), nullptr, &status);

    if (status != CL_SUCCESS)
    {
        throw CLException("Failed to create radiance buffer", status);
    }

    output_image_ = std::make_unique<cl::ImageGL>(cl_context.GetContext(), CL_MEM_WRITE_ONLY,
        GL_TEXTURE_2D, 0, gl_interop_image_, &status);

    if (status != CL_SUCCESS)
    {
        throw CLException("Failed to create output image", status);
    }

    // Create kernels
    render_kernel_ = std::make_unique<CLKernel>("src/Kernels/kernel_bvh.cl", cl_context_);
    copy_kernel_ = std::make_unique<CLKernel>("src/Kernels/kernel_copy.cl", cl_context_);

    // Setup kernels
    cl_mem radiance_buffer_mem = (*radiance_buffer_)();
    cl_mem output_image_mem = (*output_image_)();

    // Setup render kernel
    render_kernel_->SetArgument(RenderKernelArgument_t::BUFFER_OUT,
        &radiance_buffer_mem, sizeof(radiance_buffer_mem));
    render_kernel_->SetArgument(RenderKernelArgument_t::WIDTH, &width_, sizeof(std::uint32_t));
    render_kernel_->SetArgument(RenderKernelArgument_t::HEIGHT, &height_, sizeof(std::uint32_t));

    // Setup copy kernel
    copy_kernel_->SetArgument(0, &radiance_buffer_mem, sizeof(radiance_buffer_mem));
    copy_kernel_->SetArgument(1, &output_image_mem, sizeof(output_image_mem));
    copy_kernel_->SetArgument(2, &width_, sizeof(width_));
    copy_kernel_->SetArgument(3, &width_, sizeof(height_));

}

void PathTraceEstimator::Reset()
{
    // Reset sample count here
}

void PathTraceEstimator::SetCameraData(Camera const& camera)
{
    float3 origin = camera.GetOrigin();
    float3 front = camera.GetFrontVector();

    float3 right = Cross(front, camera.GetUpVector()).Normalize();
    float3 up = Cross(right, front);

    render_kernel_->SetArgument(RenderKernelArgument_t::CAM_ORIGIN, &origin, sizeof(float3));
    render_kernel_->SetArgument(RenderKernelArgument_t::CAM_FRONT, &front, sizeof(float3));
    render_kernel_->SetArgument(RenderKernelArgument_t::CAM_UP, &up, sizeof(float3));

    ///@TODO: move frame count to here
    std::uint32_t frame_count = camera.GetFrameCount();
    render_kernel_->SetArgument(RenderKernelArgument_t::FRAME_COUNT, &frame_count, sizeof(unsigned int));
    unsigned int seed = rand();
    render_kernel_->SetArgument(RenderKernelArgument_t::FRAME_SEED, &seed, sizeof(unsigned int));

    float aperture = camera.GetAperture();
    float focus_distance = camera.GetFocusDistance();
    render_kernel_->SetArgument(RenderKernelArgument_t::CAMERA_APERTURE, &aperture, sizeof(float));
    render_kernel_->SetArgument(RenderKernelArgument_t::CAMERA_FOCUS_DISTANCE, &focus_distance, sizeof(float));
}

void PathTraceEstimator::SetSceneData(Scene const& scene)
{
    // Set scene buffers
    cl_mem triangle_buffer = scene.GetTriangleBuffer();
    cl_mem node_buffer = scene.GetNodeBuffer();
    cl_mem material_buffer = scene.GetMaterialBuffer();
    cl_mem env_texture = scene.GetEnvTextureBuffer();

    render_kernel_->SetArgument(RenderKernelArgument_t::BUFFER_SCENE, &triangle_buffer, sizeof(cl_mem));
    render_kernel_->SetArgument(RenderKernelArgument_t::BUFFER_NODE, &node_buffer, sizeof(cl_mem));
    render_kernel_->SetArgument(RenderKernelArgument_t::BUFFER_MATERIAL, &material_buffer, sizeof(cl_mem));
    render_kernel_->SetArgument(RenderKernelArgument_t::ENVIRONMENT_TEXTURE, &env_texture, sizeof(cl_mem));
}

void PathTraceEstimator::Estimate()
{
    // Compute radiance
    cl_context_.ExecuteKernel(*render_kernel_, width_ * height_);

    // Copy radiance to the interop image
    cl_context_.AcquireGLObject((*output_image_)());
    cl_context_.ExecuteKernel(*copy_kernel_, width_ * height_);
    cl_context_.Finish();
    cl_context_.ReleaseGLObject((*output_image_)());

}
