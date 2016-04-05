#ifndef MATH_FUNCTIONS_HPP
#define MATH_FUNCTIONS_HPP

#define MATH_PI 3.14159265f

inline float clamp(float a, float min, float max)
{
    return (a < min) ? min : (a > max) ? max : a;
}

inline float radians(float degrees)
{
    return degrees * MATH_PI / 180.0f;
}

inline float sqr(float x)
{
    return x * x;
}

#endif // MATH_FUNCTIONS_HPP
