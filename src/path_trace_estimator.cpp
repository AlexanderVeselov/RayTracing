#include "path_trace_estimator.hpp"
#include "utils/cl_exception.hpp"
#include "Scene/scene.hpp"
#include "Scene/camera.hpp"
#include "acceleration_structure.hpp"
#include <GL/glew.h>

PathTraceEstimator::PathTraceEstimator(std::uint32_t width, std::uint32_t height,
    CLContext& cl_context, AccelerationStructure& acc_structure, cl_GLuint gl_interop_image)
    : width_(width)
    , height_(height)
    , cl_context_(cl_context)
    , acc_structure_(acc_structure)
    , gl_interop_image_(gl_interop_image)
{
    std::uint32_t num_rays = width_ * height_;

    // Create buffers and images
    cl_int status;

    radiance_buffer_ = std::make_unique<cl::Buffer>(cl_context.GetContext(), CL_MEM_READ_WRITE,
        width_ * height_ * sizeof(cl_float4), nullptr, &status);

    rays_buffer_ = std::make_unique<cl::Buffer>(cl_context.GetContext(), CL_MEM_READ_WRITE,
        num_rays * sizeof(cl_float4) * 2, nullptr, &status);

    hits_buffer_ = std::make_unique<cl::Buffer>(cl_context.GetContext(), CL_MEM_READ_WRITE,
        num_rays * sizeof(std::uint32_t) * 4, nullptr, &status);

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
    raygen_kernel_ = std::make_unique<CLKernel>("src/Kernels/raygeneration.cl", cl_context_);
    miss_kernel_ = std::make_unique<CLKernel>("src/Kernels/miss.cl", cl_context_);
    hit_surface_kernel_ = std::make_unique<CLKernel>("src/Kernels/hit_surface.cl", cl_context_);
    resolve_kernel_ = std::make_unique<CLKernel>("src/Kernels/resolve.cl", cl_context_);

    // Setup kernels
    cl_mem radiance_buffer_mem = (*radiance_buffer_)();
    cl_mem output_image_mem = (*output_image_)();
    cl_mem rays_buffer_mem = (*rays_buffer_)();
    cl_mem hits_buffer_mem = (*hits_buffer_)();

    // Setup render kernel
    raygen_kernel_->SetArgument(0, &width_, sizeof(width_));
    raygen_kernel_->SetArgument(1, &height_, sizeof(height_));
    raygen_kernel_->SetArgument(9, &rays_buffer_mem, sizeof(rays_buffer_mem));

    // Setup miss kernel
    miss_kernel_->SetArgument(0, &rays_buffer_mem, sizeof(rays_buffer_mem));
    miss_kernel_->SetArgument(1, &num_rays, sizeof(num_rays));
    miss_kernel_->SetArgument(2, &hits_buffer_mem, sizeof(hits_buffer_mem));
    miss_kernel_->SetArgument(4, &radiance_buffer_mem, sizeof(radiance_buffer_mem));

    // Setup hit surface kernel
    hit_surface_kernel_->SetArgument(0, &rays_buffer_mem, sizeof(rays_buffer_mem));
    hit_surface_kernel_->SetArgument(1, &num_rays, sizeof(num_rays));
    hit_surface_kernel_->SetArgument(2, &hits_buffer_mem, sizeof(hits_buffer_mem));
    hit_surface_kernel_->SetArgument(5, &radiance_buffer_mem, sizeof(radiance_buffer_mem));

    // Setup resolve kernel
    resolve_kernel_->SetArgument(0, &radiance_buffer_mem, sizeof(radiance_buffer_mem));
    resolve_kernel_->SetArgument(1, &output_image_mem, sizeof(output_image_mem));
    resolve_kernel_->SetArgument(2, &width_, sizeof(width_));
    resolve_kernel_->SetArgument(3, &width_, sizeof(height_));

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
    raygen_kernel_->SetArgument(2, &origin, sizeof(origin));
    raygen_kernel_->SetArgument(3, &front, sizeof(front));
    raygen_kernel_->SetArgument(4, &up, sizeof(up));

    ///@TODO: move frame count to here
    std::uint32_t frame_count = camera.GetFrameCount();
    raygen_kernel_->SetArgument(5, &frame_count, sizeof(frame_count));
    unsigned int seed = rand();
    raygen_kernel_->SetArgument(6, &seed, sizeof(seed));

    float aperture = camera.GetAperture();
    float focus_distance = camera.GetFocusDistance();
    raygen_kernel_->SetArgument(7, &aperture, sizeof(aperture));
    raygen_kernel_->SetArgument(8, &focus_distance, sizeof(focus_distance));
}

void PathTraceEstimator::SetSceneData(Scene const& scene)
{
    // Set scene buffers
    cl_mem triangle_buffer = scene.GetTriangleBuffer();
    cl_mem material_buffer = scene.GetMaterialBuffer();
    cl_mem env_texture = scene.GetEnvTextureBuffer();

    hit_surface_kernel_->SetArgument(3, &triangle_buffer, sizeof(cl_mem));
    hit_surface_kernel_->SetArgument(4, &material_buffer, sizeof(cl_mem));
    miss_kernel_->SetArgument(3, &env_texture, sizeof(cl_mem));
}

void PathTraceEstimator::Estimate()
{
    // Compute radiance
    std::uint32_t max_num_rays = width_ * height_;
    cl_context_.ExecuteKernel(*raygen_kernel_, max_num_rays);
    acc_structure_.IntersectRays(*rays_buffer_, max_num_rays, *hits_buffer_);
    cl_context_.ExecuteKernel(*miss_kernel_, max_num_rays);
    cl_context_.ExecuteKernel(*hit_surface_kernel_, max_num_rays);

    // Copy radiance to the interop image
    cl_context_.AcquireGLObject((*output_image_)());
    cl_context_.ExecuteKernel(*resolve_kernel_, width_ * height_);
    cl_context_.Finish();
    cl_context_.ReleaseGLObject((*output_image_)());

}
