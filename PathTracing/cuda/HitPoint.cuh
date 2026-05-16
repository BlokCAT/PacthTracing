#pragma once

#include "Vector.cuh"
// GPU 端交点（原名 HitPoint 跟 .hpp 冲突，改 GPUKitPoint）
class GPUKitPoint {
public:
	Vector3f hitcoord;
	int materialIdx = -1;
	Vector3f hitN;
	float distance;
	bool happened = false;
	Vector3f hitColor;  

	CUHD GPUKitPoint() : distance(3.402823e+38f) {}  // FLT_MAX
};
