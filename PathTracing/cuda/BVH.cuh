#pragma once

#include "Triangle.cuh"
#include "../AABB.hpp"

// ============================================================
//  GPUBVHNode — 扁平化 BVH 节点（索引导向，无指针）
//  子节点用整数索引，叶子存三角形区间
// ============================================================
struct GPUBVHNode {
	AABB bounds;           // 当前节点的包围盒
	int  leftChild;        // 左子节点索引（-1 表示无）
	int  rightChild;       // 右子节点索引（-1 表示无）
	int  triStart;         // 叶子：三角形在全局数组中的起始索引
	int  triCount;         // 叶子：三角形个数（0 = 内部节点）
	int  padding[2];       // 对齐到 64 字节（利于缓存行），预留
};

// ============================================================
//  迭代 BVH 遍历（GPU 端，显式栈，无递归）
//
//  参数:
//    nodes     - GPU 端扁平 BVH 节点数组
//    rootIdx   - 根节点索引（通常为 0）
//    triangles - GPU 端三角形数组
//    materials - GPU 端材质数组
//    ray       - 光线
//    hp        - 交点（输入时需初始化, 输出最近命中）
// ============================================================
CUHD inline void BVHTraverse(
	const GPUBVHNode* nodes, int rootIdx,
	const GPUTriangle* triangles,
	const GPUMaterial* materials,
	const Ray& ray, GPUKitPoint& hp)
{
	// 显式栈（BVH 深度一般不超过 64）
	int stack[64];
	int ptr = 0;
	stack[ptr++] = rootIdx;

	while (ptr > 0) {
		int nodeIdx = stack[--ptr];
		const GPUBVHNode& node = nodes[nodeIdx];

		// AABB 粗筛——不中则跳过整个子树
		if (!node.bounds.IsHitbox(ray)) continue;

		if (node.triCount > 0) {
			// ---- 叶子节点：遍历区间内所有三角形 ----
			for (int i = 0; i < node.triCount; i++) {
				TriangleIntersect(
					triangles, node.triStart + i,
					ray, hp, materials);
			}
		} else {
			// ---- 内部节点：压入子节点 ----
			if (node.rightChild >= 0) stack[ptr++] = node.rightChild;
			if (node.leftChild  >= 0) stack[ptr++] = node.leftChild;
		}
	}
}
