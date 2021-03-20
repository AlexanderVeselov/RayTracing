__kernel void KernelEntry(__global uint* ray_counter)
{
    uint global_id = get_global_id(0);

    if (global_id == 0)
    {
        ray_counter[0] = 0;
    }
}
