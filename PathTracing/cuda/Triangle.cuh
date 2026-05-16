#pragma once

#include "Vector.cuh"
#include "HitPoint.cuh"
#include "Ray.cuh"
#include "Material.cuh"     // GPUMaterial, MaterialType
#include "../AABB.hpp"      // AABB::IsHitbox (CUHD)
#include "../Tool.hpp"      // computeBarycentric3D, clamp, solveEquation (CUHD)

struct GPUTriangle {
	Vector3f v0, v1, v2;
	Vector3f n0, n1, n2;
	Vector2f uv0, uv1, uv2;
	int      materialIdx;
	Vector3f faceNormal;
	AABB     bounds;
	bool     isSmoothShading;
};

CUHD inline Vector3f TriangleGetColor(  //得到三角形的集中点颜色：需要传递整个纹理数组
	const GPUTriangle& tri, const Vector3f& hitpos,
	const GPUMaterial* materials)
{
	float b1, b2, b3;
	computeBarycentric3D(tri.v0, tri.v1, tri.v2, hitpos, b1, b2, b3);
	Vector2f hitUV = (tri.uv0 * b1) + (tri.uv1 * b2) + (tri.uv2 * b3);
	const GPUMaterial& mat = materials[tri.materialIdx];
	if (mat.isTexture) {
		return mat.Kd * (hitUV.x * hitUV.y);  // placeholder
	}
	return mat.Kd;
}

CUHD inline bool TriangleIntersect(
	const GPUTriangle* triangles, int triIdx,
	const Ray& ray, GPUKitPoint& hp,
	const GPUMaterial* materials)
{
	const GPUTriangle& tri = triangles[triIdx];

	if (!tri.bounds.IsHitbox(ray)) return false;

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

	// fill hit info
	hp.distance = t;
	hp.happened = true;
	hp.hitcoord = ray.Xt_pos(t);
	hp.materialIdx = tri.materialIdx;

	// normal
	if (!tri.isSmoothShading) {
		bool backFace = dotProduct(ray.dir, tri.faceNormal) > 0.0f;
		// light sources are single-sided (back face = no hit)
		if (backFace && materials[tri.materialIdx].islight) {
			hp.happened = false;
			return false;
		}
		hp.hitN = backFace ? (tri.faceNormal * -1.0f) : tri.faceNormal;
	} else {
		float c1, c2, c3;
		computeBarycentric3D(tri.v0, tri.v1, tri.v2, hp.hitcoord, c1, c2, c3);
		Vector3f interpN = (tri.n0 * c1) + (tri.n1 * c2) + (tri.n2 * c3);

		if (dotProduct(ray.dir, interpN) < 0.0f) {
			hp.hitN = interpN;
		} else if (materials[tri.materialIdx].mtype == REFRACT) {
			hp.hitN = interpN * -1.0f;
		} else {
			hp.happened = false;
			return false;
		}
	}

	hp.hitColor = TriangleGetColor(tri, hp.hitcoord, materials);
	return true;
}
