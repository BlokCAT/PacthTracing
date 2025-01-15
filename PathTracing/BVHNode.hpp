#pragma once

#ifndef RAYTRACING_BVHNODE_H
#define RAYTRACING_BVHNODE_H
#include "Object.hpp"
#include "AABB.hpp"
struct BVHnode
{
	BVHnode* lift = nullptr, * right = nullptr;
	Object* obj = nullptr;
	AABB nodeBox;
	int objsCount = 0;
	float area;

	BVHnode()
	{
		lift = nullptr;
		right = nullptr;
		obj = nullptr;
		area = 0;
	}
};

#endif