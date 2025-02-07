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
	BVHnode* recursiveBuildBVH(vector<Object*> objs , int tt); //��ǰ��Ҫ�����Ľڵ��
	void gethitposition(Ray &ray, BVHnode* tree  , HitPoint& hp);
	void getHitposition( Ray &ray , HitPoint &hp);
	void samplelight( float now_area , HitPoint &hp, float &pdf_L ,  BVHnode* tree);
	void SampleLight(HitPoint &hp, float &pdf_L);
};

#endif