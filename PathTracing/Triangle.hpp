#pragma once 


#ifndef RAYTRACING_TRI_H
#define RAYTRACING_TRI_H


#include "Material.hpp"
#include <iostream>
#include "Vector.hpp"
#include "Tool.hpp"
#include "Ray.hpp"
#include "HitPoint.hpp"

class Triangle 
{
public:
	Vector3f N;
	Triangle(const Vector3f& v11, const Vector3f& v22, const Vector3f& v33 , const Vector3f n)
	{
		v1 = v11;
		v2 = v22;
		v3 = v33;
		N = n;
		e1 = v2 - v1, e2 = v3 - v2, e3 = v1 - v3;
	}

	Vector3f getCenter()
	{

	}
	bool isHit(const Ray& ray)
	{

	}
	Vector3f getHitPosition(Ray& ray)
	{

	}

private:
	Vector3f v1, v2, v3;
	Vector3f e1, e2, e3;
};
#endif