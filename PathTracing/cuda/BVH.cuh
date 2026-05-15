#pragma once

#include "Triangle.cuh"
#include "../AABB.hpp"

// ============================================================
//  GPUBVHNode — 扁平 BVH 节点（索引，无指针）
//  叶子可同时含三角形区间和球体索引
// ============================================================
struct GPUBVHNode {
	AABB bounds;
	int  leftChild  = -1;
	int  rightChild = -1;
	int  triStart   = 0;
	int  triCount   = 0;       // 三角形个数（0=内部节点或纯球体叶子）
	int  sphereIdx  = -1;      // 球体索引（-1=不含球体）
	int  padding;
};

// ============================================================
//  GPUSphere — GPU 端球体数据
// ============================================================
struct GPUSphere {
	Vector3f center;
	float    radius;
	int      materialIdx;
	AABB     bounds;
};

// 球体求交（跟 CPU Boll::getHitPoint 一致）
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

// ============================================================
//  BVH 遍历（迭代，显式栈）
// ============================================================
CUHD inline void BVHTraverse(
	const GPUBVHNode* nodes, int rootIdx,
	const GPUTriangle* triangles,
	const GPUSphere*    spheres,
	const GPUMaterial* materials,
	const Ray& ray, GPUKitPoint& hp)
{
	if (rootIdx < 0 || !nodes) return;

	int stack[64];
	int ptr = 0;
	stack[ptr++] = rootIdx;

	while (ptr > 0) {
		int nodeIdx = stack[--ptr];
		const GPUBVHNode& node = nodes[nodeIdx];

		if (!node.bounds.IsHitbox(ray)) continue;

		// 三角形叶子
		for (int i = 0; i < node.triCount; i++) {
			TriangleIntersect(triangles, node.triStart + i, ray, hp, materials);
		}

		// 球体叶子
		if (node.sphereIdx >= 0) {
			SphereIntersect(spheres[node.sphereIdx], ray, hp, materials);
		}

		// 内部节点
		if (node.leftChild >= 0 || node.rightChild >= 0) {
			if (node.rightChild >= 0) stack[ptr++] = node.rightChild;
			if (node.leftChild  >= 0) stack[ptr++] = node.leftChild;
		}
	}
}
