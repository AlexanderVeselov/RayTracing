__kernel void main(__global float4* result)
{
    result[get_global_id(0)] = 1;
}
