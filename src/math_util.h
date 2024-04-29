#pragma once

#include <cstdint>

template <typename T>
inline T min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
inline T clamp(T value, T min, T max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline int16_t saturating_add(int16_t a, int16_t b)
{
    int32_t result = (int32_t)a + (int32_t)b;
    return (int16_t)clamp<int32_t>(result, INT16_MIN, INT16_MAX);
}
