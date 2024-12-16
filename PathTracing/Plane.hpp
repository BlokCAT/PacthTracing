#pragma once

#ifndef RAYTRACING_PLANE_H
#define RAYTRACING_PLANE_H

#include "Object.hpp"
#include "Vector.hpp"
#include "Material.hpp"
#include <iostream>
#include <cmath>
class Plane :public Object
{

private:
	float half_len;
	float ara;
	Vector3f Z;
	Vector3f node1, node2, node3, node4;
	Vector3f e1 , e2 , e3 , e4; //四条边，顺时针或逆时针
	AABB box;

	void setAABB()
	{
		float maxx, maxy, maxz, minx, miny, minz;
		maxx = fmax(node1.x, fmax(node2.x, fmax(node3.x, node4.x)));
		maxy = fmax(node1.y, fmax(node2.y, fmax(node3.y, node4.y)));
		maxz = fmax(node1.z, fmax(node2.z, fmax(node3.z, node4.z)));
		minx = fmin(node1.x, fmin(node2.x, fmin(node3.x, node4.x)));
		miny = fmin(node1.y, fmin(node2.y, fmin(node3.y, node4.y)));
		minz = fmin(node1.z, fmin(node2.z, fmin(node3.z, node4.z)));
		this->box.pMax = Vector3f(maxx, maxy, maxz);
		this->box.pMin = Vector3f(minx, miny, minz);
	}

	bool isInPlane(Vector3f& pos)const
	{
		Vector3f p1 = pos - node1;
		Vector3f p2 = pos - node2;
		Vector3f p3 = pos - node3;
		Vector3f p4 = pos - node4;

		Vector3f t1 = crossProduct(e1, p1);
		Vector3f t2 = crossProduct(e2, p2);
		Vector3f t3 = crossProduct(e3, p3);
		Vector3f t4 = crossProduct(e4, p4);

		ChangeO(t1, t2, t3, t4);

		float d1 = dotProduct(t1, N);
		float d2 = dotProduct(t2, N);
		float d3 = dotProduct(t3, N);
		float d4 = dotProduct(t4, N);
		if ((d1 >= 0 && d2 >= 0 && d3 >= 0 && d4 >= 0) || (d1 <= 0 && d2 <= 0 && d3 <= 0 && d4 <= 0))
			return true;
		return false;
	}

public:
	Vector3f cen, N , up;  //中心，法向量，一个垂直一条边和垂直法向量的上方向量
	float len = 0.0;
	
	Plane(Vector3f c, Vector3f n, Vector3f u, float l,Material* mm)
	{
		cen = c;
		N = n;
		up = u;
		m = mm;
		len = l;
		half_len = len / 2.0;
		N.normalized();
		up.normalized();
		Z = crossProduct(N, up).normalized();


		node1 = cen + (Z * half_len) + (up * half_len);
		node2 = cen - (Z * half_len) + (up * half_len);
		node3 = cen - (Z * half_len) - (up * half_len);
		node4 = cen + (Z * half_len) - (up * half_len);
		e1 = node2 - node1; //起点node1的边向量
		e2 = node3 - node2;
		e3 = node4 - node3;
		e4 = node1 - node4;
		ara = len * len;
		setAABB();
	}

	AABB getAABB() { return this->box; }

	void getHitPoint(Ray &ray, HitPoint &res)const
	{
		float ans = dotProduct(ray.dir, N);
		if ( ans >= 0 ) return;
		Vector3f L = cen - ray.pos;
		float t = dotProduct(L, N) / ans;
		Vector3f hit_pos = ray.pos + (ray.dir * t);

		if (!isInPlane(hit_pos))return;
		if (res.distance > t)
		{
			res.distance = t;
			res.happened = true;
			res.hitcoord = hit_pos;
			res.hitN = N;
			res.m = m;
		}
		return;
	}
	float getAra()
	{
		return ara;
	}
	void SampleLight(HitPoint &hp, float &pdf_L)
	{
		float dir_z = len * RandomFloat() , dir_u = len *  RandomFloat();
		Vector3f sameple_node = node3 + (Z * dir_z) + (up * dir_u);
		hp.happened = true;
		hp.hitcoord = sameple_node;
		hp.hitN = N;
		hp.m = m;
		pdf_L = 1 / getAra();
		if (pdf_L < 0.001)pdf_L = 0.001;
	}
};
#endif