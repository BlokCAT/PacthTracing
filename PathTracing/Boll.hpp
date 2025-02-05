#pragma once

#ifndef RAYTRACING_BOLL_H
#define RAYTRACING_BOLL_H

#include "Object.hpp"
#include "Vector.hpp"
#include "Material.hpp"
#include <iostream>


class Boll :public Object
{

public:
	Vector3f cen;
	float r;
	AABB box;
	//计算给定球表面一个点的法向量  hit - cen
	Vector3f getN(const Vector3f &facePoint)const
	{
		return facePoint - cen;
	}

	void setAABB()
	{
		//or no need
		box.pMax = Vector3f(cen.x + r, cen.y + r, cen.z + r);
		box.pMin = Vector3f(cen.x - r, cen.y - r, cen.z - r);
	}

public: 
	Boll(){}
	Boll(Vector3f c, float rr, Material* mm)
	{
		cen = c;
		r = rr;
		m = mm;
		setAABB();
	}
	void getHitPoint( Ray &ray ,  HitPoint &res) 
	{
		//std::cout << " 求交函数访问成功\n";
		float t1 = 0.0f , t2 = 0.0f;
		Vector3f o = ray.pos;
		Vector3f d = ray.dir;
		float a = dotProduct(d , d);
		float b = 2 * dotProduct(o - cen , d);
		float c = dotProduct(o - cen , o - cen) - (r * r);
		
		if (!sloveEquation(a, b, c, t1, t2))return ;
		if (t1 < 0)t1 = t2;
		if (t1 <= 0.02)return;
		if (t1 > 0.02)
		{
				res.distance = t1;
				res.happened = true;
				res.hitcoord = ray.Xt_pos(t1);
				Vector3f BollNormal = getN(res.hitcoord);
				if (dotProduct(ray.dir, BollNormal) < 0)
				{
					res.hitN = BollNormal;
				}
				else
				{
					res.hitN = BollNormal * -1;
				}
				res.m = m;
				res.hitColor = getHitColor(res.hitcoord);
				return;

		}
	}
	float getAra(){return 4.0 * M_PI * r * r;}
	AABB getAABB() { return this->box; }
	void SampleLight(HitPoint &hp, float &pdf_L)
	{
		float theta = 2.0 * M_PI * RandomFloat(), phi = M_PI * RandomFloat();
		Vector3f dir(std::cos(phi), std::sin(phi) * std::cos(theta), std::sin(phi) * std::sin(theta));
		dir.normalized();
		hp.hitcoord = cen + (dir * r);
		hp.hitN = getN(hp.hitcoord);
		hp.m = m;	
		pdf_L = 1.0f / getAra();
		return;
	}
	Vector3f getHitColor(const Vector3f& hitpos)
	{
		return m->Kd;
	}
};
#endif