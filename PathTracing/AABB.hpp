#pragma once
#ifndef RAYTRACING_AABB_H
#define RAYTRACING_AABB_H
#include "Vector.hpp"
#include "Ray.hpp"

class AABB
{
private :
	void my_swap(float& t1, float& t2)
	{
		float t = t1;
		t1 = t2;
		t2 = t;
	}
public:
	Vector3f pMax, pMin;
	AABB ( ):
		pMax(0),
		pMin(0)
	{}
	AABB(Vector3f pmax, Vector3f pmin)
	{
		pMax = pmax; 
		pMin = pmin;
	}
	CUHD bool IsHitbox(const Ray &ray)
	{
		float t_minx, t_miny, t_minz, t_maxx, t_maxy, t_maxz;
		float time_in, time_out;

		// X 轴
		if (fabsf(ray.dir.x) < 1e-6f) {
			if (ray.pos.x < pMin.x || ray.pos.x > pMax.x) return false;
			t_minx = -1e30f; t_maxx = 1e30f;
		} else {
			t_minx = (pMin.x - ray.pos.x) / ray.dir.x;
			t_maxx = (pMax.x - ray.pos.x) / ray.dir.x;
			if (t_minx > t_maxx) my_swap(t_minx, t_maxx);
		}

		// Y 轴
		if (fabsf(ray.dir.y) < 1e-6f) {
			if (ray.pos.y < pMin.y || ray.pos.y > pMax.y) return false;
			t_miny = -1e30f; t_maxy = 1e30f;
		} else {
			t_miny = (pMin.y - ray.pos.y) / ray.dir.y;
			t_maxy = (pMax.y - ray.pos.y) / ray.dir.y;
			if (t_miny > t_maxy) my_swap(t_miny, t_maxy);
		}

		// Z 轴
		if (fabsf(ray.dir.z) < 1e-6f) {
			if (ray.pos.z < pMin.z || ray.pos.z > pMax.z) return false;
			t_minz = -1e30f; t_maxz = 1e30f;
		} else {
			t_minz = (pMin.z - ray.pos.z) / ray.dir.z;
			t_maxz = (pMax.z - ray.pos.z) / ray.dir.z;
			if (t_minz > t_maxz) my_swap(t_minz, t_maxz);
		}

		time_in = fmax(t_minx, fmax(t_miny, t_minz));
		time_out = fmin(t_maxx, fmin(t_maxy, t_maxz));

		if (time_in <= time_out && time_out >= 0)return true;
		return false;
	}

	Vector3f getCen()
	{
		return (pMax + pMin) / 2;
	}
};
inline AABB AmalgamateTowBox(const AABB& a, const AABB& b)
{
	float max_x, min_x, max_y, min_y, max_z, min_z;
	max_x = fmax(a.pMax.x, b.pMax.x);
	max_y = fmax(a.pMax.y, b.pMax.y);
	max_z = fmax(a.pMax.z, b.pMax.z);
	min_x = fmin(a.pMin.x, b.pMin.x);
	min_y = fmin(a.pMin.y, b.pMin.y);
	min_z = fmin(a.pMin.z, b.pMin.z);
	AABB res(Vector3f(max_x, max_y, max_z), Vector3f(min_x, min_y, min_z));
	return res;
}
#endif 
