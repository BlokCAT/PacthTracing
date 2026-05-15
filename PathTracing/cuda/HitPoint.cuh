#pragma once

#include "Vector.cuh"
#include <cfloat>

class HitPoint {
public:
	Vector3f hitcoord;
	int materialIdx = -1;
	Vector3f hitN;
	float distance;
	bool happened = false;
	Vector3f hitColor;

	CUHD HitPoint() : distance(FLT_MAX) {}
};
