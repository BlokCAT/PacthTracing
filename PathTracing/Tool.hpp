#pragma once
#ifndef PATHTRACING_TOOL_H
#define PATHTRACING_TOOL_H

#include "cuda/Vector.cuh"    // CUHD 宏 + Vector3f/Vector2f
#include "cuda/Tool.cuh"      // 新版 computeBarycentric3D（7参数） + RNGState
#include <cmath>
#include <random>
#include <iostream>
#include <tuple>
#undef M_PI
#define M_PI 3.141592653589793f
#undef RussianRoulette
#define RussianRoulette 0.8

CUHD inline float clamp(const float &lo, const float &hi, const float &v) {
	return fmaxf(lo, fminf(hi, v));
}

CUHD inline float TriangleArea(const Vector3f& v1, const Vector3f& v2, const Vector3f& v3) {
	Vector3f e1 = v2 - v1, e2 = v3 - v1;
	return fabsf(0.5f * crossProduct(e1, e2).len());
}

// 旧版 computeBarycentric3D（tuple 返回，CPU 兼容）
CUHD inline std::tuple<float, float, float> computeBarycentric3D(
	const Vector3f& v1, const Vector3f& v2, const Vector3f& v3, const Vector3f& hit)
{
	float AllArea = TriangleArea(v1, v2, v3);
	return { TriangleArea(hit, v2, v3) / AllArea,
			TriangleArea(hit, v1, v3) / AllArea,
			TriangleArea(hit, v2, v1) / AllArea };
}

CUHD inline bool sloveEquation(float a, float b, float c, float &t1, float &t2) {
	float deta = sqrtf(b * b - (4.0f * a * c));
	if (deta < 0.00001f) return false;
	if (-0.00001f <= deta && deta < 0.00001f) {
		t1 = -b / (2.0f * a);
		t2 = t1;
		return true;
	}
	if (deta >= 0.00001f) {
		t1 = (-b + deta) / (2.0f * a);
		t2 = (-b - deta) / (2.0f * a);
		if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
		return true;
	}
	return false;
}

// ---- CPU-only: 随机数 + 进度条 ----
inline static float RandomFloat() {
	static std::random_device d;
	static std::mt19937 res(d());
	static std::uniform_real_distribution<float> D(0.f, 1.f);
	return D(res);
}

CUHD inline void ChangeO(Vector3f &t1, Vector3f &t2, Vector3f &t3, Vector3f &t4) {
	if (t1.x == -0.0f) t1.x = 0.0f;
	if (t1.y == -0.0f) t1.y = 0.0f;
	if (t1.z == -0.0f) t1.z = 0.0f;
	if (t2.x == -0.0f) t2.x = 0.0f;
	if (t2.y == -0.0f) t2.y = 0.0f;
	if (t2.z == -0.0f) t2.z = 0.0f;
	if (t3.x == -0.0f) t3.x = 0.0f;
	if (t3.y == -0.0f) t3.y = 0.0f;
	if (t3.z == -0.0f) t3.z = 0.0f;
	if (t4.x == -0.0f) t4.x = 0.0f;
	if (t4.y == -0.0f) t4.y = 0.0f;
	if (t4.z == -0.0f) t4.z = 0.0f;
}


inline void UpdateProgress(float progress)
{
	int barWidth = 70;

	std::cout << "[";
	int pos = barWidth * progress;


	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	std::cout << "] " << int(progress * 100.0) << " %\r";
	std::cout.flush();
};
#endif // PATHTRACING_TOOL_H
