#if !defined(PROTOTYPE_INTRINSICS_H)

// Todo: convert to platform efficient versions and remove math.h
#include "math.h"

inline int32_t RoundFloatToInt32(float Number) {
    int32_t Result = (int32_t)(Number + 0.5f);
    return Result;
}

inline uint32_t RoundFloatToUInt32(float Number) {
    uint32_t Result = (uint32_t)(Number + 0.5f);
    return Result;
}

inline int32_t FloorFloatToInt32(float Number) {
    int32_t Result = (int32_t)floorf(Number);
    return(Result);
}

inline int32_t TruncateFloatToInt32(float Number) {
    int32_t Result = (int32_t)Number;
    return Result;
}

inline float Sin(float Angle) {
    float Result = sinf(Angle);
    return Result;
}

inline float Cos(float Angle) {
    float Result = cosf(Angle);
    return Result;
}

inline float ATan2(float Y, float X) {
    float Result = atan2f(Y, X);
    return Result;
}

#define PROTOTYPE_INTRINSICS_H
#endif