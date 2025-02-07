#pragma once
#ifndef RAYTRACING_OBJECT_H
#define RAYTRACING_OBJECT_H
#include "Vector.hpp"
#include "Tool.hpp"
#include "Ray.hpp"
#include "HitPoint.hpp"
#include "AABB.hpp"



class Object
{
public:
	Object (){}
	~Object() {}
	Material* m;
	virtual void getHitPoint( Ray &ray, HitPoint &res) = 0;
	virtual float getAra() = 0;
	virtual AABB getAABB() = 0;
	virtual void setAABB() = 0;
	virtual void SampleLight(HitPoint& hp, float& pdf_L) = 0;
	virtual Vector3f getHitColor(const Vector3f &hitpos) = 0;
};

#endif