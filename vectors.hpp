#ifndef VECTORS_HPP
#define VECTORS_HPP

#include <cmath>

struct float3
{
	float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	float3(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z) {}
	float3(float _val) : x(_val), y(_val), z(_val) {}

	float length() { return sqrt(x*x + y*y + z*z); }
	float3 normalize() { return float3(x / length(), y / length(), z / length()); }

	// Scalar operators
	float3 operator+ (float scalar) { return float3(x + scalar, y + scalar, z + scalar); }
	float3 operator- (float scalar) { return float3(x - scalar, y - scalar, z - scalar); }
	float3 operator* (float scalar) { return float3(x * scalar, y * scalar, z * scalar); }
	float3 operator/ (float scalar) { return float3(x / scalar, y / scalar, z / scalar); }

	// Vector operators
	float3 operator+ (const float3 &other) { return float3(x + other.x, y + other.y, z + other.z); }
	float3 operator- (const float3 &other) { return float3(x - other.x, y - other.y, z - other.z); }
		
	float x, y, z;

private:
	// Unused
	float w;

};

#endif // VECTORS_HPP
