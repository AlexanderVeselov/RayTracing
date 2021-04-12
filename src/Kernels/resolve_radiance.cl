__kernel void KernelEntry
(
    uint width,
    uint height,
    __global float4* radiance,
    __global uint* sample_counter,
    __write_only image2d_t result
)
{
    uint global_id = get_global_id(0);

    uint sample_count = sample_counter[0];

    int x = global_id % width;
    int y = global_id / width;
    write_imagef(result, (int2)(x, y), radiance[global_id] / (float)sample_count);
}