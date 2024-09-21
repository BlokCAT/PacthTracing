#pragma once 
#include "Vector.hpp"
#include "Material.hpp"

#ifndef RAYTRACING_INTERSECTION_H
#define RAYTRACING_INTERSECTION_H
class HitPoint
{
public:
	HitPoint():
		distance(9999)
	{}
	Vector3f hitcoord;
	Material* m = NULL;  //���ʿ��Է���diffuse��ɫ���� �������ɫ
	Vector3f hitN;
	float distance ;
	bool happened;
};
#endif