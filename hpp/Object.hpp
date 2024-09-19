#pragma once
#ifndef RAYTRACING_OBJECT_H
#define RAYTRACING_OBJECT_H
#include "Vector.hpp"
#include "Tool.hpp"
#include "Ray.hpp"
#include "HitPoint.hpp"



class Object
{
public:

	Object (){}
	Material* m;
	virtual void getHitPoint( Ray &ray, HitPoint &res)const = 0;
	virtual float getAra() = 0;
	virtual void SampleLight(HitPoint &hp, float &pdf_L) = 0;
};

#endif