#pragma once 
#include "Vector.hpp"
#include "Material.hpp"
#include <limits>

#ifndef RAYTRACING_INTERSECTION_H
#define RAYTRACING_INTERSECTION_H
class HitPoint
{
public:
	HitPoint():
		distance(std::numeric_limits<long long>::max())
	{}
	Vector3f hitcoord;
	Material* m = nullptr;  //���ʿ��Է���diffuse��ɫ���� �������ɫ
	Vector3f hitN;
	float distance ;
	bool happened = false;
	Vector3f hitColor;
};
#endif