#pragma once

#include "Triangle.cuh"
#include "Sphere.cuh"
#include "../AABB.hpp"

// ============================================================
//  GPUBVHNode — 扁平 BVH 节点（索引，无指针）
//  叶子可同时含三角形区间和球体索引
// ============================================================
struct GPUBVHNode {
	AABB bounds;           // 当前节点及子树的包围盒
	int  leftChild  = -1;
	int  rightChild = -1;
	int  triStart   = 0;   // 三角形数组起始索引
	int  triCount   = 0;   // 三角形个数（0 = 内部节点或纯球体叶子）
	int  sphereIdx  = -1;  // 球体索引（-1 = 不含球体）
	int  padding;          // 对齐
};

// ============================================================
//  BVH 遍历（迭代，显式栈，无递归）
// ============================================================
CUHD inline void BVHTraverse(
	const GPUBVHNode* nodes, int rootIdx,
	const TriangleGeo*  geo,
	const TriangleMeta* meta,
	const GPUSphere*    spheres,
	const GPUMaterial* materials,
	const Ray& ray, GPUKitPoint& hp)
{
	if (rootIdx < 0 || !nodes) return;

	int stack[64];
	int ptr = 0;
	stack[ptr++] = rootIdx; //先把根节点入栈

	while (ptr > 0) { //当栈不空
		int nodeIdx = stack[--ptr];
		const GPUBVHNode& node = nodes[nodeIdx];

		if (!node.bounds.IsHitbox(ray)) continue;

		// 三角形叶子
		for (int i = 0; i < node.triCount; i++) {
			TriangleIntersect(geo, node.triStart + i, meta, ray, hp, materials);
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
