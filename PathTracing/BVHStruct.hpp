#pragma once

#ifndef RAYTRACING_BVHSTRUCT_H
#define RAYTRACING_BVHSTRUCT_H
#include "Object.hpp"
#include <vector>
#include "BVHNode.hpp"
using namespace std;

class BVHstruct
{
public:
	vector<Object*> objects;
	BVHnode* root = NULL; 
	BVHstruct(){}

	BVHstruct(vector<Object*> objs)
	{
		objects = objs;
	}
	~BVHstruct(){}
	int getnextTurn(int now);
	void BuiltBVH( int t);
	BVHnode* recursiveBuildBVH(vector<Object*> objs , int tt); //当前需要创建的节点的
	void gethitposition(Ray &ray, BVHnode* tree  , HitPoint& hp);
	void getHitposition( Ray &ray , HitPoint &hp);
	void samplelight( float now_area , HitPoint &hp, float &pdf_L ,  BVHnode* tree);
	void SampleLight(HitPoint &hp, float &pdf_L);
};

#endif