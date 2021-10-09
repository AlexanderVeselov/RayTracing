__kernel void ResetRadiance
(
    // Input
    uint width,
    uint height,
    // Output
    __global float4* radiance_buffer
)
{
    uint pixel_idx = get_global_id(0);

    if (pixel_idx >= width * height)
    {
        return;
    }

    radiance_buffer[pixel_idx] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
}
