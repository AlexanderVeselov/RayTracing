#ifndef MATHLIB_HPP
#define MATHLIB_HPP

#include <cmath>
#include <cstring>
#include <iostream>

const float MATH_PI = 3.141592654f;
const float MATH_2PI = 6.283185307f;
const float MATH_1DIVPI = 0.318309886f;
const float MATH_1DIV2PI = 0.159154943f;
const float MATH_PIDIV2 = 1.570796327f;
const float MATH_PIDIV4 = 0.785398163f;

struct float3
{
    float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    float3(float val) : x(val), y(val), z(val) {}
    float3() : x(0), y(0), z(0) {}

    float length() const { return sqrt(x*x + y*y + z*z); }
    float3 normalize() const { return float3(x / length(), y / length(), z / length()); }

    // Scalar operators
    float3 operator+ (float scalar) { return float3(x + scalar, y + scalar, z + scalar); }
    float3 operator- (float scalar) { return float3(x - scalar, y - scalar, z - scalar); }
    float3 operator* (float scalar) { return float3(x * scalar, y * scalar, z * scalar); }
    float3 operator/ (float scalar) { return float3(x / scalar, y / scalar, z / scalar); }

    // Vector operators
    friend float3 operator+ (const float3 &lhs, const float3 &rhs) { return float3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); }
    friend float3 operator- (const float3 &lhs, const float3 &rhs) { return float3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }

    float3& operator+= (const float3 &other) { x += other.x; y += other.y; z += other.z; return *this; }
    float3& operator*= (const float  &other) { x *= other;   y *= other;   z *= other;   return *this; }
    float3& operator-= (const float3 &other) { x -= other.x; y -= other.y; z -= other.z; return *this; }

    friend float3 operator- (const float3& vec) { return float3(-vec.x, -vec.y, -vec.z); }

    //float operator[] (size_t i) const { return i == 0 ? x : (i == 1 ? y : z); }
    float& operator[] (size_t i) { return i == 0 ? x : (i == 1 ? y : z); }
    const float& operator[] (size_t i) const { return i == 0 ? x : (i == 1 ? y : z); }
    friend std::ostream& operator<< (std::ostream &os, const float3 &vec) { return os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")"; }
    friend float3 operator* (const float3& a, float b) { return float3(a.x * b, a.y * b, a.z * b); }

    bool IsZero() const { return x == 0.0f && y == 0.0f && z == 0.0f; }

    float x, y, z;

private:
    // Used for align to 4 bytes
    float w;

};

#define float3_aligned _declspec(align(16)) float3

struct float2
{
    float2(float x, float y) : x(x), y(y) {}
    float2(float val) : x(val), y(val) {}
    float2() : x(0), y(0) {}

    float length() const { return sqrt(x*x + y*y); }
    float2 normalize() const { return float2(x / length(), y / length()); }

    // Scalar operators
    float2 operator+ (float scalar) { return float2(x + scalar, y + scalar); }
    float2 operator- (float scalar) { return float2(x - scalar, y - scalar); }
    float2 operator* (float scalar) { return float2(x * scalar, y * scalar); }
    float2 operator/ (float scalar) { return float2(x / scalar, y / scalar); }

    // Vector operators
    float2 operator+ (const float2 &other) { return float2(x + other.x, y + other.y); }
    float2 operator- (const float2 &other) { return float2(x - other.x, y - other.y); }
    friend float2 operator+ (const float2 &lhs, const float2 &rhs) { return float2(lhs.x + rhs.x, lhs.y + rhs.y); }
    friend float2 operator- (const float2 &lhs, const float2 &rhs) { return float2(lhs.x - rhs.x, lhs.y - rhs.y); }

    float2 operator+= (const float2 &other) { x += other.x; y += other.y; return *this; }
    float2 operator*= (const float  &other) { x *= other;   y *= other;   return *this; }
    float2 operator-= (const float2 &other) { x -= other.x; y -= other.y; return *this; }

    float2 operator- () { return float2(-x, -y); }

    float operator[] (size_t i) const { return i == 0 ? x : y; }
    //float& operator[] (size_t i) { return i == 0 ? x : y; }

    float x, y;

};

inline float3 cross(const float3& a, const float3& b)
{
    return float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline float dot(const float3& a, const float3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float distance(const float3& a, const float3& b)
{
    return (b - a).length();
}

struct Triangle;

struct Bounds3
{
public:
    Bounds3(float3 min, float3 max) :
        min(min), max(max)
    {}
    
    bool Intersects(const Triangle &triangle) const;
    void Project(float3 axis, float &mins, float &maxs) const;

    float3 min;
    float3 max;

};


struct Matrix
{
    static Matrix LookAtLH(const float3& eye, const float3& target, const float3& up = float3(0.0f, 0.0f, 1.0f));
    static Matrix LookAtRH(const float3& eye, const float3& target, const float3& up = float3(0.0f, 0.0f, 1.0f));
    static Matrix PerspectiveFovLH(float fov, float aspect, float nearZ, float farZ);
    static Matrix PerspectiveFovRH(float fov, float aspect, float nearZ, float farZ);
    static Matrix OrthoLH(float width, float height, float nearZ, float farZ);
    static Matrix OrthoRH(float width, float height, float nearZ, float farZ);
    static Matrix Zero();
    static Matrix Identity();
    static Matrix Translation(const float3& translation);
    static Matrix Translation(float x, float y, float z);
    static Matrix RotationAxis(const float3& axis, float angle);
    static Matrix RotationAxisAroundPoint(const float3& axis, const float3& point, float angle);
    static Matrix Scaling(float scalex, float scaley, float scalez);

    // Constructors
    Matrix() { memset(m, 0, sizeof(float) * 16); }
    Matrix(float matrix[4][4]) { memcpy(m, matrix, sizeof(float) * 16); }

    Matrix(float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33)
    {
        m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
        m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
        m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
        m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
    }

    Matrix(const Matrix& other)
    {
        m[0][0] = other.m[0][0]; m[0][1] = other.m[0][1]; m[0][2] = other.m[0][2]; m[0][3] = other.m[0][3];
        m[1][0] = other.m[1][0]; m[1][1] = other.m[1][1]; m[1][2] = other.m[1][2]; m[1][3] = other.m[1][3];
        m[2][0] = other.m[2][0]; m[2][1] = other.m[2][1]; m[2][2] = other.m[2][2]; m[2][3] = other.m[2][3];
        m[3][0] = other.m[3][0]; m[3][1] = other.m[3][1]; m[3][2] = other.m[3][2]; m[3][3] = other.m[3][3];
    }

    // Methods
    Matrix Inverse() const;
    Matrix Transpose() const
    {
        return Matrix(m[0][0], m[1][0], m[2][0], m[3][0],
            m[0][1], m[1][1], m[2][1], m[3][1],
            m[0][2], m[1][2], m[2][2], m[3][2],
            m[0][3], m[1][3], m[2][3], m[3][3]);
    }

    // Operators
    Matrix operator*(const Matrix& other);
    Matrix& operator*= (const Matrix& other);
    float3  operator* (const float3& vec);
    Matrix& operator= (const Matrix& other)
    {
        m[0][0] = other.m[0][0]; m[0][1] = other.m[0][1]; m[0][2] = other.m[0][2]; m[0][3] = other.m[0][3];
        m[1][0] = other.m[1][0]; m[1][1] = other.m[1][1]; m[1][2] = other.m[1][2]; m[1][3] = other.m[1][3];
        m[2][0] = other.m[2][0]; m[2][1] = other.m[2][1]; m[2][2] = other.m[2][2]; m[2][3] = other.m[2][3];
        m[3][0] = other.m[3][0]; m[3][1] = other.m[3][1]; m[3][2] = other.m[3][2]; m[3][3] = other.m[3][3];
        return *this;
    }

    float m[4][4];
};

template <typename T>
inline T clamp(T value, T min, T max)
{
    return (value < min) ? min : ((value > max) ? max : value);
}

#endif // MATHLIB_HPP