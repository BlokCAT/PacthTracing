#pragma once 
#include "Vector.hpp"
#include "Material.hpp"

#ifndef RAYTRACING_INTERSECTION_H
#define RAYTRACING_INTERSECTION_H
class HitPoint
{
public:
	HitPoint() {}
	Vector3f hitcoord;
	Material* m;  //���ʿ��Է���diffuse��ɫ���� �������ɫ
	Vector3f hitN;
	float distance = 9999;
	bool happened = false;
};
#endif