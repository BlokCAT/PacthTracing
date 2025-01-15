#pragma once
#include <sstream>
#include <iostream>
#include <string>
#include <cmath>

class Vector3f
{
private:
    std::string myConvert(float Num) const {
        return std::to_string(Num);
    }
public:
    float x = 0.0, y = 0.0, z = 0.0;
    Vector3f() :x(0.0), y(0.0), z(0.0) {}
    Vector3f(float a) :x(a), y(a), z(a) {}
    Vector3f(float a, float b, float c) :x(a), y(b), z(c) {}

    Vector3f operator * (const Vector3f& v) const { return Vector3f(x * v.x, y * v.y, z * v.z); }
    Vector3f operator * (float r) const { return Vector3f(x * r, y * r, z * r); }
    Vector3f operator / (float r) const { return Vector3f(x / r, y / r, z / r); }
    Vector3f operator + (const Vector3f& v1) const { return Vector3f(x + v1.x, y + v1.y, z + v1.z); }
    Vector3f operator - (const Vector3f& v1) const { return Vector3f(x - v1.x, y - v1.y, z - v1.z); }

    float len() const { return std::sqrt(x * x + y * y + z * z); }
    Vector3f normalized() const {
        float n = len();
        if (n == 0) return *this;
        return Vector3f(x / n, y / n, z / n);
    }

    std::string showVec() const
    {
        return  "(" + myConvert(x) + "," + myConvert(y) + "," + myConvert(z) + " )";
    }
};

inline float dotProduct(const Vector3f& a, const Vector3f& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3f crossProduct(const Vector3f& a, const Vector3f& b)
{
    return Vector3f(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}