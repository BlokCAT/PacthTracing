#pragma once

#ifndef RAYTRACING_BVHSTRUCT_H
#define RAYTRACING_BVHSTRUCT_H
#include "Object.hpp"
#include <vector>
#include <unordered_map>
#include "BVHNode.hpp"
#include "cuda/BVH.cuh"   // GPUBVHNode 结构体
using namespace std;

class BVHstruct
{
public:
	vector<Object*> objects;
	BVHnode* root = nullptr;
	BVHstruct(){}

	BVHstruct(vector<Object*> objs)
	{
		objects = objs;
	}
	~BVHstruct()
	{
		if (root){
			deleteNode(root);
		}
	}
	void deleteNode(BVHnode* node);
	int getnextTurn(int now);
	void BuiltBVH( int t);
	BVHnode* recursiveBuildBVH(vector<Object*> objs , int tt); //当前需要创建的节点的
	void gethitposition(Ray &ray, BVHnode* tree  , HitPoint& hp);
	void getHitposition( Ray &ray , HitPoint &hp);
	void samplelight( float now_area , HitPoint &hp, float &pdf_L ,  BVHnode* tree);
	void SampleLight(HitPoint &hp, float &pdf_L);

	// ---- GPU 迁移：拍平 BVH 为扁平数组 ----
	std::vector<struct GPUBVHNode> flattenBVH(
		const std::unordered_map<Object*, std::pair<int,int>>& triMap,
		const std::unordered_map<Object*, int>& sphereMap) const;
};

#endif