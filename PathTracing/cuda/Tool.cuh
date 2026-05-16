#pragma once

#include "Vector.cuh"
#include <cmath>

// ---- 常量（跟 Tool.hpp 保持一致）----
#undef M_PI
#define M_PI 3.141592653589793f
#undef RussianRoulette
#define RussianRoulette 0.8

// ---- 注意：clamp / TriangleArea / solveEquation / ChangeO ----
// 这些函数留在 Tool.hpp，GPU 端会用 Tool.hpp 的版本（纯数学，加 CUHD 后双向可用）。
// Tool.cuh 只放 Tool.hpp 里没有的新内容。

// ============================================================
//  重心坐标 — 改掉 std::tuple，改用 3 个输出参数
//  （新签名，不会跟 Tool.hpp 的旧版冲突）
// ============================================================
CUHD inline void computeBarycentric3D(
	const Vector3f& v1, const Vector3f& v2, const Vector3f& v3,
	const Vector3f& hit,
	float& b1, float& b2, float& b3)
{
	// 内联三角形面积，避免依赖 Tool.hpp
	auto triArea = [](const Vector3f& a, const Vector3f& b, const Vector3f& c) -> float {
		Vector3f e1 = b - a, e2 = c - a;
		return 0.5f * crossProduct(e1, e2).len();
	};
	float AllArea = triArea(v1, v2, v3);
	b1 = triArea(hit, v2, v3) / AllArea;
	b2 = triArea(hit, v1, v3) / AllArea;
	b3 = triArea(hit, v2, v1) / AllArea;
}

// ============================================================
//  随机数 
// ============================================================
struct RNGState {
	unsigned long long state;
};

// 简单 LCG 随机数（GPU 可用，后续可换 curand）
CUHD inline float gpuRand(RNGState& rng) {
	rng.state = rng.state * 1103515245ULL + 12345ULL;
	return (float)((rng.state >> 16) & 0x7FFF) / 32768.0f;
}
