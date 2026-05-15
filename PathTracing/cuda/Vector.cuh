#pragma once

#include <cmath>

// ---- 非 CUDA 编译器兼容：将 CUHD 定义为空 ----
#ifdef __CUDACC__
#define CUHD __host__ __device__
#else
#define CUHD
#endif

// ============================================================
//  Vector3f  --  GPU/CPU 共享
// ============================================================
class Vector3f {
public:
	float x = 0.0f, y = 0.0f, z = 0.0f;

	CUHD Vector3f() : x(0.0f), y(0.0f), z(0.0f) {}
	CUHD Vector3f(float a) : x(a), y(a), z(a) {}
	CUHD Vector3f(float a, float b, float c) : x(a), y(b), z(c) {}

	CUHD Vector3f operator * (const Vector3f& v) const { return Vector3f(x * v.x, y * v.y, z * v.z); }
	CUHD Vector3f operator * (float r) const { return Vector3f(x * r, y * r, z * r); }
	CUHD Vector3f operator / (float r) const { return Vector3f(x / r, y / r, z / r); }
	CUHD Vector3f operator + (const Vector3f& v) const { return Vector3f(x + v.x, y + v.y, z + v.z); }
	CUHD Vector3f operator - (const Vector3f& v) const { return Vector3f(x - v.x, y - v.y, z - v.z); }

	CUHD float len() const { return sqrtf(x * x + y * y + z * z); }
	CUHD Vector3f normalized() const {
		float n = len();
		if (n == 0.0f) return *this;
		return Vector3f(x / n, y / n, z / n);
	}
};

// ============================================================
//  全局向量函数
// ============================================================
CUHD inline float dotProduct(const Vector3f& a, const Vector3f& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

CUHD inline Vector3f crossProduct(const Vector3f& a, const Vector3f& b) {
	return Vector3f(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

// ============================================================
//  Vector2f  --  GPU/CPU 共享
// ============================================================
class Vector2f {
public:
	float x = 0.0f, y = 0.0f;

	CUHD Vector2f() : x(0.0f), y(0.0f) {}
	CUHD Vector2f(float a) : x(a), y(a) {}
	CUHD Vector2f(float a, float b) : x(a), y(b) {}

	CUHD Vector2f operator * (const Vector2f& v) const { return Vector2f(x * v.x, y * v.y); }
	CUHD Vector2f operator * (float r) const { return Vector2f(x * r, y * r); }
	CUHD Vector2f operator / (float r) const { return Vector2f(x / r, y / r); }
	CUHD Vector2f operator + (const Vector2f& v) const { return Vector2f(x + v.x, y + v.y); }
	CUHD Vector2f operator - (const Vector2f& v) const { return Vector2f(x - v.x, y - v.y); }

	CUHD float len() const { return sqrtf(x * x + y * y); }
	CUHD Vector2f normalized() const {
		float n = len();
		if (n == 0.0f) return *this;
		return Vector2f(x / n, y / n);
	}
};

