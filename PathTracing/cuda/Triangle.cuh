#pragma once

#include "Vector.cuh"
#include "HitPoint.cuh"
#include "Ray.cuh"
#include "../AABB.hpp"      // AABB::IsHitbox 已加 CUHD
#include "../Tool.hpp"      // computeBarycentric3D, clamp, solveEquation 已加 CUHD

// ============================================================
//  GPUTriangle — GPU 端三角形数据（纯 POD，平铺数组）
// ============================================================
struct GPUTriangle {
	Vector3f v0, v1, v2;        // 三个顶点
	Vector3f n0, n1, n2;        // 三个顶点的法线
	Vector2f uv0, uv1, uv2;     // 三个顶点的 UV
	int      materialIdx;       // 材质数组索引
	Vector3f faceNormal;        // 面法线（预计算）
	AABB     bounds;            // 包围盒（预计算）
	bool     isSmoothShading;
};

// ============================================================
//  三角形命中点颜色（GPU 端，暂跳过纹理直接用 Kd）
// ============================================================
CUHD Vector3f TriangleGetColor(
	const GPUTriangle& tri, const Vector3f& hitpos,
	const GPUMaterial* materials)
{
	float b1, b2, b3;
	computeBarycentric3D(tri.v0, tri.v1, tri.v2, hitpos, b1, b2, b3);

	Vector2f hitUV = (tri.uv0 * b1) + (tri.uv1 * b2) + (tri.uv2 * b3);

	const GPUMaterial& mat = materials[tri.materialIdx];
	if (mat.isTexture) {
		// TODO: 纹理采样（CUDA texture object），暂用 Kd
		return mat.Kd * (hitUV.x * hitUV.y);  // 占位
	}
	return mat.Kd;
}

// ============================================================
//  Möller-Trumbore 求交（GPU 端，去掉 std::tuple/std::array）
// ============================================================
CUHD bool TriangleIntersect(
	const GPUTriangle* triangles, int triIdx,
	const Ray& ray, HitPoint& hp,
	const GPUMaterial* materials)
{
	const GPUTriangle& tri = triangles[triIdx];

	// ---- AABB 粗筛 ----
	if (!tri.bounds.IsHitbox(ray)) return false;

	// ---- Möller-Trumbore ----
	Vector3f E1 = tri.v1 - tri.v0;
	Vector3f E2 = tri.v2 - tri.v0;
	Vector3f S  = ray.pos - tri.v0;
	Vector3f S1 = crossProduct(ray.dir, E2);
	Vector3f S2 = crossProduct(S, E1);

	float denom = dotProduct(S1, E1);
	if (denom > -0.0001f && denom < 0.0001f) return false;

	float invDenom = 1.0f / denom;
	float t  = invDenom * dotProduct(S2, E2);
	float b1 = invDenom * dotProduct(S1, S);
	float b2 = invDenom * dotProduct(S2, ray.dir);

	if (b1 < 0.0f || b1 >= 1.0f) return false;
	if (b2 < 0.0f || b2 >= 1.0f) return false;
	if (b1 + b2 >= 1.0f) return false;

	if (t <= 0.0f || t >= hp.distance) return false;

	// ---- 命中！填充 HitPoint ----
	hp.distance = t;
	hp.happened = true;
	hp.hitcoord = ray.Xt_pos(t);
	hp.materialIdx = tri.materialIdx;

	// 法线计算
	if (!tri.isSmoothShading) {
		// 平面着色：用面法线
		if (dotProduct(ray.dir, tri.faceNormal) > 0.0f)
			hp.hitN = tri.faceNormal * -1.0f;
		else
			hp.hitN = tri.faceNormal;
	} else {
		// 平滑着色：重心坐标插值顶点法线
		float c1, c2, c3;
		computeBarycentric3D(tri.v0, tri.v1, tri.v2, hp.hitcoord, c1, c2, c3);
		Vector3f interpN = (tri.n0 * c1) + (tri.n1 * c2) + (tri.n2 * c3);

		if (dotProduct(ray.dir, interpN) < 0.0f) {
			hp.hitN = interpN;
		} else if (materials[tri.materialIdx].mtype == REFRACT) {
			// 折射体接受背面命中，翻转法线
			hp.hitN = interpN * -1.0f;
		} else {
			hp.happened = false;
			return false;
		}
	}

	// 命中点颜色
	hp.hitColor = TriangleGetColor(tri, hp.hitcoord, materials);
	return true;
}
