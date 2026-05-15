#pragma once
#include "Vector.cuh"

class Ray {
public:
	Vector3f pos;
	Vector3f dir;

	CUHD Ray() : pos(), dir() {}
	CUHD Ray(const Vector3f& p, const Vector3f& d) : pos(p), dir(d) {}
	CUHD Vector3f Xt_pos(float t) const { return pos + (dir * t); }
};
