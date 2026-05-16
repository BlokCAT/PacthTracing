#pragma once

#include "Vector.cuh"
#include "Ray.cuh"
#include "HitPoint.cuh"
#include "Material.cuh"

// ============================================================
//  GPUSphere — GPU 端球体数据（纯 POD）
// ============================================================
struct GPUSphere {
	Vector3f center;
	float    radius;
	int      materialIdx;   // 材质数组索引
	AABB     bounds;        // 预计算包围盒
};

// ============================================================
//  球体求交（跟 CPU Boll::getHitPoint 一致）
//  解析公式：|O + t*D - C|^2 = r^2 → 二次方程
// ============================================================
CUHD inline bool SphereIntersect(
	const GPUSphere& sphere,
	const Ray& ray, GPUKitPoint& hp,
	const GPUMaterial* materials)
{
	Vector3f oc = ray.pos - sphere.center;
	float a = dotProduct(ray.dir, ray.dir);
	float b = 2.0f * dotProduct(oc, ray.dir);
	float c = dotProduct(oc, oc) - sphere.radius * sphere.radius;

	float det = b * b - 4.0f * a * c;
	if (det < 0.0f) return false;

	float sqrtDet = sqrtf(det);
	float t1 = (-b - sqrtDet) / (2.0f * a);
	float t2 = (-b + sqrtDet) / (2.0f * a);

	// 取近交点，跳过自交（t ≤ 0.02）
	float t = (t1 > 0.02f) ? t1 : t2;
	if (t <= 0.02f || t >= hp.distance) return false;

	Vector3f hitCoord = ray.Xt_pos(t);
	Vector3f normal = hitCoord - sphere.center;

	hp.distance = t;
	hp.happened = true;
	hp.hitcoord = hitCoord;
	hp.hitN = (dotProduct(ray.dir, normal) < 0.0f) ? normal : (normal * -1.0f);
	hp.materialIdx = sphere.materialIdx;
	hp.hitColor = materials[sphere.materialIdx].Kd;
	return true;
}
