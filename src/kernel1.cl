__kernel void main(__global float4* result, __global int* random_int, uint width, uint height)
{
	float invWidth = 1 / (float)(width), invHeight = 1 / (float)(height);

	float x = (float)(get_global_id(0) % width);
	float y = (float)(get_global_id(0) / width);

	x = x * invWidth;
    result[get_global_id(0)] = x;
}
