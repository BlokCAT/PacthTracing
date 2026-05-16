#pragma once

#include "Vector.cuh"
#include "HitPoint.cuh"
#include "Ray.cuh"
#include "Material.cuh"
#include "../AABB.hpp"
#include "../Tool.hpp"

// ============================================================
//  TriangleGeo — 热数据：求交时需要，BVH 遍历频繁读取
//  v0/v1/v2: Möller-Trumbore 算法用
//  bounds:   BVH 叶子 AABB 粗筛用
//  总大小: 3×12 + 24 = 60 字节（原 GPUTriangle 140 字节）
// ============================================================
struct TriangleGeo {
	Vector3f v0, v1, v2;
	AABB     bounds;
};

// ============================================================
//  TriangleMeta — 冷数据：只在命中后才读
//  法线用于平滑着色，UV 用于纹理，materialIdx 索引材质数组
// ============================================================
struct TriangleMeta {
	Vector3f n0, n1, n2;       // 顶点法线
	Vector2f uv0, uv1, uv2;    // 纹理 UV
	Vector3f faceNormal;       // 面法线
	int      materialIdx;      // 材质索引
	bool     isSmoothShading;
};

// ============================================================
//  TriangleGetColor — 重心坐标插值颜色（命中后调用）
// ============================================================
CUHD inline Vector3f TriangleGetColor(
	const TriangleGeo* geo, const TriangleMeta* meta, int triIdx,
	const Vector3f& hitpos,
	const GPUMaterial* materials)
{
	const TriangleGeo&  g = geo[triIdx];
	const TriangleMeta& m = meta[triIdx];
	float b1, b2, b3;
	computeBarycentric3D(g.v0, g.v1, g.v2, hitpos, b1, b2, b3);
	Vector2f hitUV = (m.uv0 * b1) + (m.uv1 * b2) + (m.uv2 * b3);
	const GPUMaterial& mat = materials[m.materialIdx];
	if (mat.isTexture) {
		return mat.Kd * (hitUV.x * hitUV.y);
	}
	return mat.Kd;
}

// ============================================================
//  TriangleIntersect — Möller-Trumbore 求交（只收热数据）
// ============================================================
CUHD inline bool TriangleIntersect(
	const TriangleGeo* geo, int triIdx,
	const TriangleMeta* meta,
	const Ray& ray, GPUKitPoint& hp,
	const GPUMaterial* materials)
{
	const TriangleGeo& g = geo[triIdx];

	// AABB 粗筛
	if (!g.bounds.IsHitbox(ray)) return false;

	// Möller-Trumbore
	Vector3f E1 = g.v1 - g.v0;
	Vector3f E2 = g.v2 - g.v0;
	Vector3f S  = ray.pos - g.v0;
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
	hp.materialIdx = meta[triIdx].materialIdx;

	// 法线计算（读冷数据 meta）
	const TriangleMeta& m = meta[triIdx];
	if (!m.isSmoothShading) {
		bool backFace = dotProduct(ray.dir, m.faceNormal) > 0.0f;
		if (backFace && materials[m.materialIdx].islight) {
			hp.happened = false;
			return false;
		}
		hp.hitN = backFace ? (m.faceNormal * -1.0f) : m.faceNormal;
	} else {
		float c1, c2, c3;
		computeBarycentric3D(g.v0, g.v1, g.v2, hp.hitcoord, c1, c2, c3);
		Vector3f interpN = (m.n0 * c1) + (m.n1 * c2) + (m.n2 * c3);

		if (dotProduct(ray.dir, interpN) < 0.0f) {
			hp.hitN = interpN;
		} else if (materials[m.materialIdx].mtype == REFRACT) {
			hp.hitN = interpN * -1.0f;
		} else {
			hp.happened = false;
			return false;
		}
	}

	hp.hitColor = TriangleGetColor(geo, meta, triIdx, hp.hitcoord, materials);
	return true;
}
