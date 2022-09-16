#if !defined(PROTOTYPE_MATH_H)

union v2 {
    struct {
        float X, Y;
    };
    float E[2];
};

inline v2 V2(float X, float Y) {
    v2 Result;

    Result.X = X;
    Result.Y = Y;

    return Result;
}

inline v2 operator*(float A, v2 B) {
    v2 Result;

    Result.X = A*B.X;
    Result.Y = A*B.Y;

    return Result;
}

inline v2 operator*(v2 B, float A) {
    v2 Result = A*B;

    return Result;
}

inline v2 &operator*=(v2 &B, float A) {
    B = A * B;

    return B;
}

inline v2 operator-(v2 A) {
    v2 Result;

    Result.X = -A.X;
    Result.Y = -A.Y;

    return Result;
}

inline v2 operator+(v2 A, v2 B) {
    v2 Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}

inline v2 &operator+=(v2 &A, v2 B) {
    A = A + B;

    return A;
}

inline v2 operator-(v2 A, v2 B) {
    v2 Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}

inline float Square(float A) {
    float Result = A*A;

    return Result;
}

inline float InnerProduct(v2 A, v2 B) {
    float Result = A.X*B.X + A.Y*B.Y;

    return Result;
}

inline float LengthSq(v2 A) {
    float Result = InnerProduct(A, A);

    return Result;
}

#define PROTOTYPE_MATH_H
#endif