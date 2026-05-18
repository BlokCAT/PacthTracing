#pragma once

#include "Vector.cuh"
#include "Ray.cuh"

// ============================================================
//  Camera — 透视相机（GPU/CPU 共享）
//  generateRay: 像素坐标 → 世界空间光线
// ============================================================
struct Camera {
	Vector3f position;
	Vector3f lookAt;
	Vector3f up;
	float    fov = 40.0f;       // 垂直 FOV（度）
	float    focusDist = 0.6f;  // 成像平面距离

	// 默认构造：匹配当前硬编码参数
	CUHD Camera() : position(0.3f, -0.8f, 8.0f), lookAt(0.3f, -0.8f, 9.0f),
	                up(0.0f, 1.0f, 0.0f), fov(40.0f), focusDist(0.6f) {}

	// 生成像素 (i, j) 对应的光线
	CUHD Ray generateRay(int i, int j, int width, int height) const {
		// 构建 camera 坐标系
		Vector3f forward = lookAt - position;
		float fLen = forward.len();
		forward = (fLen < 0.0001f) ? Vector3f(0,0,1) : (forward / fLen);

		Vector3f camRight = crossProduct(forward, up);
		float rLen = camRight.len();
		camRight = (rLen < 0.0001f) ? Vector3f(1,0,0) : (camRight / rLen);

		Vector3f camUp = crossProduct(camRight, forward);
		float uLen = camUp.len();
		camUp = (uLen < 0.0001f) ? Vector3f(0,1,0) : (camUp / uLen);

		// 像素 → 屏幕坐标 → 世界方向
		float halfH = tanf(fov * 0.5f * (float)M_PI / 180.0f);
		float eryPixer = halfH / (float)(height / 2);
		float x = ((float)j + 0.5f) * eryPixer - halfH * (float)width / (float)height;
		float y = (1.0f - ((float)i + 0.5f) / (float)height * 2.0f) * halfH;

		Vector3f dir = (forward * focusDist) + (camRight * x) + (camUp * y);
		return Ray(position, dir);
	}
};
