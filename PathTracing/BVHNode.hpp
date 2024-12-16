#pragma once

#ifndef RAYTRACING_BVHNODE_H
#define RAYTRACING_BVHNODE_H
#include "Object.hpp"
#include "AABB.hpp"
struct BVHnode
{
	BVHnode* lift = NULL, * right = NULL;
	Object* obj = NULL;  
	AABB nodeBox;
	int objsCount = 0;
	float area;

	BVHnode()
	{
		lift = NULL;
		right = NULL;
		obj = NULL;
		area = 0;
	}
};

#endif