__kernel void KernelEntry
(
    __global float4* input,
    __write_only image2d_t result,
    uint width,
    uint height
)
{
    uint global_id = get_global_id(0);

    int x = global_id % width;
    int y = global_id / width;
    write_imagef(result, (int2)(x, y), input[global_id]);
}
